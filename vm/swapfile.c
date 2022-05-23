#include <swapfile.h>
#include <spinlock.h>
#include <vm.h>
#include <vfs.h>
#include <types.h>
#include <kern/fcntl.h>
#include <uio.h>
#include <vnode.h>
#include <lib.h>

static int SWAP_PAGES = MAX_SWAP_SIZE/PAGE_SIZE;

static struct vnode* swapfile;
static struct spinlock swap_lock = SPINLOCK_INITIALIZER;
static char file_name[] = "SWAPFILE";
static struct swapfile_entry *swap_entries = NULL;

void swapfile_init(void){
    int result,i;
    
    swap_entries = kmalloc(sizeof(struct swapfile_entry) * SWAP_PAGES);

    result = vfs_open(file_name, O_RDWR|O_CREAT|O_TRUNC, 0, &swapfile);
    if(result){
        panic("swap file open error");
    }

    for(i=0; i<SWAP_PAGES; i++){
        swap_entries[i].full = 0;
        swap_entries[i].vaddr = 0;
        swap_entries[i].as = NULL;
    }
    
    spinlock_init(&swap_lock);
}
void swap_in(paddr_t paddr, vaddr_t vaddr, struct addrspace* as){
    int i, offset, result;
    struct iovec iov;
    struct uio uio;
    
    spinlock_acquire(&swap_lock);
    for(i=0; i<SWAP_PAGES; i++){
        if (swap_entries[i].full == 0)
            break;
    }
    if(i == SWAP_PAGES){
        panic("Out of swap space");
    }
    
    offset = i * PAGE_SIZE;

    swap_entries[i].full = 1;
    swap_entries[i].vaddr = vaddr;
    swap_entries[i].as = as;
    spinlock_release(&swap_lock);
    
    vaddr = PADDR_TO_KVADDR(paddr);
    uio_kinit(&iov, &uio, (void *) vaddr, PAGE_SIZE, offset, UIO_WRITE);
    result = VOP_WRITE(swapfile, &uio);
    if(result){
        panic("write swap");
    }
}

void swap_out(paddr_t paddr, vaddr_t vaddr){
    int i, offset, result;
    struct iovec iov;
    struct uio uio;
    
    spinlock_acquire(&swap_lock);
    for(i=0; i<SWAP_PAGES; i++){
        if(swap_entries[i].vaddr == vaddr)
            break;
    }

    if(i == SWAP_PAGES){
        panic("wrong vaddr");
    }
    
    offset = i * PAGE_SIZE;

    swap_entries[i].full = 0;
    swap_entries[i].vaddr = 0;
    swap_entries[i].as = NULL;
    spinlock_release(&swap_lock);
    
    vaddr = PADDR_TO_KVADDR(paddr);
    uio_kinit(&iov, &uio, (void *) vaddr, PAGE_SIZE, offset, UIO_READ);
    result = VOP_READ(swapfile, &uio);
    if(result){
        panic("write swap");
    }

}
void swapfile_free(struct addrspace* as){
    int i;
    spinlock_acquire(&swap_lock);
    for(i=0; i<SWAP_PAGES; i++){
        if(swap_entries[i].as == as){
            swap_entries[i].full = 0;
            swap_entries[i].vaddr = 0;
            swap_entries[i].as = NULL;
        }          
    }
    spinlock_release(&swap_lock);
}

void swapfile_close(void){
    vfs_close(swapfile);
}