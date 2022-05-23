#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <cpu.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <coremap.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>
#include <elf.h>
#include <uio.h>
#include <vnode.h>
#include <swapfile.h>
#include <vmstats.h>

#define NICEVM_STACKPAGES 18

/* Under nicevm, always have 72k of user stack */
/* (this must be > 64K so argument blocks of size ARG_MAX will fit) */

static int n_vm_fault = 0;

int tlb_get_rr_victim(void);
void tlb_insert(struct addrspace *as, vaddr_t faultaddress, paddr_t paddr, int i);
void tlb_invalid_entry(struct addrspace *as, int victim);

void vm_bootstrap(void)
{
	ramFrame_bootstrap();
}

void vm_shootdown(void)
{
	print_stats();
	swapfile_close();
}

void vm_tlbshootdown(const struct tlbshootdown *ts)
{
   (void)ts;
   panic("dumbvm tried to do tlb shootdown?!\n");
}

/*
 * Round-Robin TLB Replacement
 */
int
tlb_get_rr_victim(void) {
	int victim;
	static unsigned int next_victim = 0;

	victim = next_victim;
	next_victim = (next_victim + 1) % NUM_TLB;
	return victim;
}

void
tlb_insert(struct addrspace *as, vaddr_t faultaddress, paddr_t paddr, int i)
{
	uint32_t ehi, elo;
	int spl, k, index;
	
	spl = splhigh();
	// inserimento in TLB
	for (k=0; k<NUM_TLB; k++) {
		tlb_read(&ehi, &elo, k);
		if (elo & TLBLO_VALID) {
		 	continue;	// salta un'iterazione del loop
		}
		ehi = faultaddress;
		if (as->pt->entries[i].type == READONLY)
			elo = paddr | TLBLO_VALID;
		else
			elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		//elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		//DEBUG(DB_VM, "segmento corto: 0x%x -> 0x%x\n", faultaddress, paddr);
		tlb_write(ehi, elo, k);
		splx(spl);
		break;
	}
	// se TLB piena: rimpiazzo
	if(k == NUM_TLB){
		inc_TLBFaults_with_replace();
		
		index = tlb_get_rr_victim();
		ehi = faultaddress;
		if(as->pt->entries[i].type == READONLY)
			elo = paddr | TLBLO_VALID;
		else
			elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		//DEBUG(DB_VM, "Rimpiazzo: 0x%x -> 0x%x\n", faultaddress, paddr);
		tlb_write(ehi, elo, index);
		splx(spl);
	}
	else {
		inc_TLBFaults_with_free();
	}
}

void
tlb_invalid_entry(struct addrspace *as, int victim)
{
	uint32_t ehi, elo;
	int spl, k;

	spl = splhigh();
	for (k=0; k<NUM_TLB; k++) {
		tlb_read(&ehi, &elo, k);
		if (ehi != as->pt->entries[victim].vaddr) {
		 	continue;	// salta un'iterazione del loop
		}
		tlb_write(TLBHI_INVALID(k), TLBLO_INVALID(), k);
		break;
	}
	splx(spl);	
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	n_vm_fault++;
	paddr_t paddr;
	int i, j, k;
	int addr_offset, victim, result, load = 0;
	struct addrspace *as;
	struct iovec iov;
	struct uio ku;

	Elf_Phdr ph;

	faultaddress &= PAGE_FRAME;

	DEBUG(DB_VM, "fault: 0x%x - type: %d - N=%d\n", faultaddress, faulttype, n_vm_fault);

	KASSERT(faultaddress != 0);

	switch (faulttype) {
	    case VM_FAULT_READONLY:
			return 1;
	    case VM_FAULT_READ:
	    case VM_FAULT_WRITE:
		break;
	    default:
		return EINVAL;
	}

	if (curproc == NULL) {
		/*
		 * No process. This is probably a kernel fault early
		 * in boot. Return EFAULT so as to panic instead of
		 * getting into an infinite faulting loop.
		 */
		return EFAULT;
	}

	as = proc_getas();
	if (as == NULL) {
		/*
		 * No address space set up. This is probably also a
		 * kernel fault early in boot.
		 */
		return EFAULT;
	}

	/* Assert that the address space has been set up properly. */
	KASSERT(as->pt->npages != 0);
	for(i=0; i<as->pt->npages; i++){
		KASSERT(as->pt->entries[i].vaddr != 0);
		KASSERT((as->pt->entries[i].vaddr & PAGE_FRAME) == as->pt->entries[i].vaddr);
		if(as->pt->entries[i].validbit){
			KASSERT((as->pt->entries[i].paddr & PAGE_FRAME) == as->pt->entries[i].paddr);
		}
	}

	inc_TLBFaults();

	// cerca il faultaddress nella page table
	for(i=0; i<as->pt->npages; i++){
		if(as->pt->entries[i].vaddr == faultaddress){
			// trovato nella page table
			if(as->pt->entries[i].validbit){
				// pagina già scritta
				if(!as->pt->entries[i].swapbit){
					// pagina presente in memoria
					inc_TLBReloads();
					paddr = as->pt->entries[i].paddr;
					break;
				}
				else{
					// pagina presente in swapfile: swapout
					paddr = getppages(1);
					if(paddr == 0){
						// memoria piena: swapin
						do{
							// seleziona vittima
							victim = pt_get_rr_victim(as->pt);
						} while(victim == i || as->pt->entries[victim].validbit == 0 || as->pt->entries[victim].swapbit == 1);
						swap_in(as->pt->entries[victim].paddr, as->pt->entries[victim].vaddr,as);
						inc_Swapfile_writes();
						as->pt->entries[victim].swapbit = 1;
						paddr = as->pt->entries[victim].paddr; // paddr vittima usato dalla nuova pagina
						as->pt->entries[victim].paddr = 0;
						
						// invalida la entry vittima in TLB
						tlb_invalid_entry(as,victim);
					}
					swap_out(paddr, as->pt->entries[i].vaddr);
					inc_PageFaults_swapfile();
					inc_PageFaults_disk();
					as->pt->entries[i].swapbit = 0;
					as->pt->entries[i].paddr = paddr;
				}	
			}
			else{
				// Lettura da ELF: alloca posto in memoria, carica pagina, aggiorna address space e carica in TLB
				paddr = getppages(1);
				if(paddr == 0){
					// memoria piena: swapin
					do{
						// seleziona vittima
						victim = pt_get_rr_victim(as->pt);
					} while(victim == i || as->pt->entries[victim].validbit == 0 || as->pt->entries[victim].swapbit == 1);
					swap_in(as->pt->entries[victim].paddr, as->pt->entries[victim].vaddr,as);
					inc_Swapfile_writes();
					as->pt->entries[victim].swapbit = 1;
					paddr = as->pt->entries[victim].paddr; // paddr vittima usato dalla nuova pagina
					as->pt->entries[victim].paddr = 0;
					// invalida la entry vittima in TLB
					tlb_invalid_entry(as,victim);
				}
				
				as->pt->entries[i].paddr = paddr;
				KASSERT(as->pt->entries[i].paddr != 0);
				as->pt->entries[i].validbit = 1;
				as_zero_region(as->pt->entries[i].paddr, 1);
				// se il faultaddress è di stack non si legge l'ELF
				if(as->pt->entries[i].type != STACK){
					load=1;
				}
				else{
					inc_PageFaults_zeroed();
				}
				break;
			}
		}
	}

	// lettura ELF e caricamento segmento
	if(load){
		// lettura executable header per ciclare sui program header
		for (j=0; j<as->eh.e_phnum; j++) {
			off_t offset = as->eh.e_phoff + j*as->eh.e_phentsize;
			uio_kinit(&iov, &ku, &ph, sizeof(ph), offset, UIO_READ);

			result = VOP_READ(as->v, &ku);
			if (result) {
				return ENOMEM;
			}
			switch (ph.p_type) {
	 	    	case PT_NULL: /* skip */ continue;
	 	    	case PT_PHDR: /* skip */ continue;
	 	    	case PT_MIPS_REGINFO: /* skip */ continue;
	 	    	case PT_LOAD: break;
	 	    	default:
	 			return ENOEXEC;
	 		}
			if((ph.p_vaddr & PAGE_FRAME) <= faultaddress 
					&& ((ph.p_vaddr & PAGE_FRAME) + ph.p_memsz) >= faultaddress ){
					break;
			}
		}

		// segmento corto: non si legge ELF file e si carica una pagina vuota in TLB
		if(faultaddress > ((ph.p_vaddr & PAGE_FRAME) + ph.p_filesz) ){
			// inserimento in TLB
			tlb_insert(as, faultaddress, paddr, i);
			inc_PageFaults_zeroed();
			return 0;
		}

		// calcolo per capire se la pagina si riempie
		if((ph.p_filesz + (ph.p_vaddr & PAGE_FRAME) ) > (faultaddress + PAGE_SIZE)){
			result = PAGE_SIZE;
		}
		else {
			result = (ph.p_filesz + (ph.p_vaddr & PAGE_FRAME) ) - faultaddress; 
		}
		
		// calcolo offset nel segmento
		k = 0;
		while(faultaddress != PAGE_SIZE * k + (ph.p_vaddr & PAGE_FRAME)){
			k++;
		}
		// se prima pagina del segmento e faultaddress è diverso dal suo vaddr
		if(k == 0 && faultaddress != ph.p_vaddr){
			addr_offset = ph.p_vaddr - faultaddress;
		}
		else
		{
			addr_offset = 0;
		}
		
		
		// leggi segmento
		iov.iov_ubase = (void *) PADDR_TO_KVADDR(paddr + addr_offset);
		iov.iov_len = PAGE_SIZE;		 // length of the memory space
		ku.uio_iov = &iov;
		ku.uio_iovcnt = 1;
		ku.uio_resid = result;          // amount to read from the file
		ku.uio_offset = ph.p_offset + k * PAGE_SIZE;
		ku.uio_segflg = UIO_SYSSPACE;
		ku.uio_rw = UIO_READ;
		ku.uio_space = NULL;

		result = VOP_READ(as->v, &ku);
		
		inc_PageFaults_disk();
		inc_PageFaults_elf();
	}
	
	/* make sure it's page-aligned */
	KASSERT((paddr & PAGE_FRAME) == paddr);

	// inserimento in TLB
	tlb_insert(as, faultaddress, paddr, i);
	
	return 0;
}
