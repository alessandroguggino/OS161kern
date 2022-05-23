#include <vmstats.h>
#include <types.h>
#include <lib.h>
#include <spinlock.h>

static struct spinlock stats_lock = SPINLOCK_INITIALIZER;
static struct vmstats stats;


void init_stats(void)
{
    spinlock_acquire(&stats_lock);
    stats.TLBFaults = 0;
    stats.TLBFaults_with_free = 0;
    stats.TLBFaults_with_replace = 0;
    stats.TLBInvalidations = 0;
    stats.TLBReloads = 0;
    stats.PageFaults_zeroed = 0;
    stats.PageFaults_elf = 0;
    stats.PageFaults_disk = 0;
    stats.PageFaults_swapfile = 0;
    stats.Swapfile_writes = 0;
    spinlock_release(&stats_lock);
}

void inc_TLBFaults(void)
{
    spinlock_acquire(&stats_lock);
    stats.TLBFaults++;
    spinlock_release(&stats_lock);
}

void inc_TLBFaults_with_free(void)
{
    spinlock_acquire(&stats_lock);
    stats.TLBFaults_with_free++;
    spinlock_release(&stats_lock);
}

void inc_TLBFaults_with_replace(void)
{
    spinlock_acquire(&stats_lock);
    stats.TLBFaults_with_replace++;
    spinlock_release(&stats_lock);
}

void inc_TLBInvalidations(void)
{
    spinlock_acquire(&stats_lock);
    stats.TLBInvalidations++;
    spinlock_release(&stats_lock);
}

void inc_TLBReloads(void){
    spinlock_acquire(&stats_lock);
    stats.TLBReloads++;
    spinlock_release(&stats_lock);
}

void inc_PageFaults_zeroed(void){
    spinlock_acquire(&stats_lock);
    stats.PageFaults_zeroed++;
    spinlock_release(&stats_lock);
}
void inc_PageFaults_elf(void){
    spinlock_acquire(&stats_lock);
    stats.PageFaults_elf++;
    spinlock_release(&stats_lock);
}
void inc_PageFaults_disk(void){
    spinlock_acquire(&stats_lock);
    stats.PageFaults_disk++;
    spinlock_release(&stats_lock);
}
void inc_PageFaults_swapfile(void){
    spinlock_acquire(&stats_lock);
    stats.PageFaults_swapfile++;
    spinlock_release(&stats_lock);
}
void inc_Swapfile_writes(void){
    spinlock_acquire(&stats_lock);
    stats.Swapfile_writes++;
    spinlock_release(&stats_lock);
}
void print_stats(void){
    spinlock_acquire(&stats_lock);
    
    if((stats.TLBFaults_with_free + stats.TLBFaults_with_replace) != stats.TLBFaults){
        kprintf("Warning! TLB faults with free + TLB faults with replace should be equal to TLB faults\n");
    }
    if((stats.TLBReloads + stats.PageFaults_disk + stats.PageFaults_zeroed) != stats.TLBFaults){
        kprintf("Warning! TLB reloads + Page faults disk + Page faults zeroed should be equal to TLB faults\n");
    }
    if((stats.PageFaults_elf + stats.PageFaults_swapfile) != stats.PageFaults_disk){
        kprintf("Warning! Page faults from ELF + Page faults from Swapfile should be equal to Page faults (Disk)\n");
    }
    
    kprintf("\nVM STATS:\n");
    kprintf("TLB Faults = %d\n", stats.TLBFaults);
    kprintf("TLB Faults with free = %d\n", stats.TLBFaults_with_free);
    kprintf("TLB Faults with replace = %d\n", stats.TLBFaults_with_replace);
    kprintf("TLB Invalidations = %d\n", stats.TLBInvalidations);
    kprintf("TLB Reloads = %d\n", stats.TLBReloads);
    kprintf("Page Faults (Zeroed) = %d\n", stats.PageFaults_zeroed);
    kprintf("Page Faults (Disk) = %d\n", stats.PageFaults_disk);
    kprintf("Page Faults from ELF = %d\n", stats.PageFaults_elf);
    kprintf("Page Faults from Swapfile = %d\n", stats.PageFaults_swapfile);
    kprintf("Swapfile writes = %d\n\n", stats.Swapfile_writes);

    spinlock_release(&stats_lock);
}