#ifndef _PT_H_
#define _PT_H_

#include <types.h>
#include "opt-paging.h"

#if OPT_PAGING

#define PT_ENTRIES 2500

struct pt_entry
{
   vaddr_t vaddr;
   paddr_t paddr;
   int validbit; // 0 invalid, 1 valid
   int swapbit;  // 0 not in swapfile, 1 in swapfile
   int type; // 0 text, 1 data, 2 stack, -1 not defined
};

struct page_table
{
   struct pt_entry entries[PT_ENTRIES];
   int npages;
};

struct page_table* pt_init(void);
void pt_free(struct page_table *pt);
int pt_get_rr_victim(struct page_table *pt);

#endif
#endif /* _PT_H_ */