/*
 * Virtual Memory Manager (VMM) API
 *
 * Provides the structures and function prototypes necessary for managing
 * x86 paging, page directories, and virtual-to-physical address translation.
 */
#ifndef VMM_H
#define VMM_H
#include <stdint.h>

// Page Table Entry (PTE) Flags
// These flags control access permissions and caching behavior for memory pages.
#define I86_PTE_PRESENT  0x1  // Indicates if the page is currently in physical memory
#define I86_PTE_WRITABLE 0x2  // Determines if the page can be written to (read/write vs read-only)
#define I86_PTE_USER     0x4  // Controls access level (user mode vs supervisor/kernel mode)
#define I86_PTE_ACCESSED 0x20 // Set by the CPU when the page is read or written
#define I86_PTE_DIRTY    0x40 // Set by the CPU when the page is written to

#define PAGE_SIZE 4096

// Represents an x86 page directory.
// The structure contains 1024 entries, each pointing to a page table.
// It must be aligned on a 4KB boundary because the CPU expects the lower 12 bits
// of the physical address to be zero (used for flags).
typedef struct {
    uint32_t tablesPhysical[1024];
} __attribute__((aligned(4096))) page_directory_t;

void                             init_vmm();
void                             vmm_map_page(void *phys, void *virt, int flags);
void                             vmm_unmap_page(void *virt);
void                             vmm_flush_tlb_entry(void *addr);
page_directory_t                *vmm_create_address_space();
void              vmm_map_page_in_dir(page_directory_t *dir, void *phys, void *virt, int flags);
void              vmm_switch_directory(page_directory_t *dir);
page_directory_t *vmm_get_current_directory();
void             *vmm_get_phys(uint32_t virt);
extern void       vmm_load_pd(uint32_t *addr);
extern uint32_t   get_cr3();

#endif