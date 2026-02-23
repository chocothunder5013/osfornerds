/* src/mm/vmm.h */
#ifndef VMM_H
#define VMM_H

#include <stdint.h>

// Page Table Entry flags
#define I86_PTE_PRESENT 0x1
#define I86_PTE_WRITABLE 0x2
#define I86_PTE_USER 0x4
#define I86_PTE_ACCESSED 0x20
#define I86_PTE_DIRTY 0x40

#define PAGE_SIZE 4096

// Struct for a Page Directory
// In x86, this is just an array of 1024 32-bit physical addresses (plus flags)
typedef struct
{
    uint32_t tablesPhysical[1024];
} __attribute__((aligned(4096))) page_directory_t;
// --- Core VMM API ---
void init_vmm();
void vmm_map_page(void *phys, void *virt, int flags);
void vmm_unmap_page(void *virt);
void vmm_flush_tlb_entry(void *addr);

// --- Multi-Process Support ---
page_directory_t *vmm_create_address_space();
void vmm_map_page_in_dir(page_directory_t *dir, void *phys, void *virt, int flags);
void vmm_switch_directory(page_directory_t *dir);
page_directory_t *vmm_get_current_directory();
void *vmm_get_phys(uint32_t virt); // Helper for debugging

// Assembly Helpers
extern void vmm_load_pd(uint32_t *addr);
extern uint32_t get_cr3();

#endif