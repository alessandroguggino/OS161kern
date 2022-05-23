#ifndef _VMSTATS_H_
#define _VMSTATS_H_

#include "opt-paging.h"

#if OPT_PAGING

struct vmstats
{
    int TLBFaults;
    int TLBFaults_with_free;
    int TLBFaults_with_replace;
    int TLBInvalidations;
    int TLBReloads;
    int PageFaults_zeroed;
    int PageFaults_elf;
    int PageFaults_disk;
    int PageFaults_swapfile;
    int Swapfile_writes;
};

void init_stats(void);
void inc_TLBFaults(void);
void inc_TLBFaults_with_free(void);
void inc_TLBFaults_with_replace(void);
void inc_TLBInvalidations(void);
void inc_TLBReloads(void);
void inc_PageFaults_zeroed(void);
void inc_PageFaults_elf(void);
void inc_PageFaults_disk(void);
void inc_PageFaults_swapfile(void);
void inc_Swapfile_writes(void);
void print_stats(void);

#endif
#endif /* _VMSTATS_H_ */