#ifndef _SWAPFILE_H_
#define _SWAPFILE_H_

#include "opt-paging.h"

#if OPT_PAGING

#include <types.h>
#include <addrspace.h>

#define MAX_SWAP_SIZE 9*1024*1024 // 9 MB

struct swapfile_entry
{
    vaddr_t vaddr;
    int full;
    struct addrspace* as;
   
};

void swapfile_init(void);
void swap_in(paddr_t paddr, vaddr_t vaddr, struct addrspace* as);
void swap_out(paddr_t paddr, vaddr_t vaddr);
void swapfile_free(struct addrspace* as);
void swapfile_close(void);

#endif
#endif /* _SWAPFILE_H_ */