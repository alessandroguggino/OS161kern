#include <types.h>
#include <lib.h>
#include <pt.h>

struct page_table * pt_init(void){
    int i;
    struct page_table *pt;
    pt = kmalloc(sizeof(struct page_table));
    pt->npages = 0;
    for(i=0; i<PT_ENTRIES; i++){
        pt->entries[i].vaddr = 0;
        pt->entries[i].paddr = 0;
        pt->entries[i].validbit = 0;
        pt->entries[i].swapbit = 0;
        pt->entries[i].type = -1;
    }
    return pt;  
        
}

void pt_free(struct page_table *pt){
    kfree(pt);
}

// selezione vittima tramite Round-Robin
int pt_get_rr_victim(struct page_table *pt) {
	int victim;
	static unsigned int next_swap_victim = 0;

	victim = next_swap_victim;
	next_swap_victim = (next_swap_victim + 1) % pt->npages;
	return victim;
}