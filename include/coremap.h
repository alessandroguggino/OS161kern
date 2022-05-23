#ifndef _COREMAP_H_
#define _COREMAP_H_

#include <types.h>
#include "opt-paging.h"

#if OPT_PAGING

void ramFrame_bootstrap(void);
paddr_t getfreeppages(unsigned long npages);
paddr_t getppages(unsigned long npages);
int freeppages(paddr_t addr, unsigned long npages);
#endif

#endif /* _COREMAP_H_ */