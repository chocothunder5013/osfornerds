/*
 * Virtual Memory Manager (VMM)
 *
 * The VMM sets up and manages x86 paging, which translates virtual addresses
 * into physical addresses. It handles page directories and page tables.
 * Paging provides memory protection and allows each process to have its own
 * isolated address space.
 */
#include "vmm.h"

extern void      *pmm_alloc_block();
extern void       pmm_free_block(void *p);
extern void       serial_log(char *str);
extern void      *memset(void *ptr, int value, uint32_t num);

page_directory_t *current_directory = 0;
page_directory_t *kernel_directory  = 0;

// Invalidates a specific Translation Lookaside Buffer (TLB) entry.
// The TLB caches page translations. When a page table entry changes, 
// the CPU must be told to invalidate the old cached mapping.
void              vmm_flush_tlb_entry(void *addr) {
    __asm__ volatile("invlpg (%0)" ::"r"(addr) : "memory");
}

// Loads a page directory into the CR3 register and enables paging.
// The CR0 register's paging bit (bit 31) is set to enable the MMU.
void vmm_load_pd(uint32_t *addr) {
    __asm__ volatile("mov %0, %%cr3" ::"r"(addr));
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0" ::"r"(cr0));
}

// Gets the current page directory physical address from CR3.
uint32_t get_cr3() {
    uint32_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

// Sets the CR3 register to point to a new page directory without modifying CR0.
void set_cr3(uint32_t pd) {
    __asm__ volatile("mov %0, %%cr3" ::"r"(pd));
}

// Maps a physical memory frame to a virtual address within a specific page directory.
// This function allocates a new page table if one does not already exist for the requested virtual address.
void vmm_map_page_in_dir(page_directory_t *dir, void *phys, void *virt, int flags) {
    // A 32-bit virtual address is split into a 10-bit page directory index, 
    // a 10-bit page table index, and a 12-bit offset within the page.
    uint32_t pdindex = (uint32_t)virt >> 22;
    uint32_t ptindex = ((uint32_t)virt >> 12) & 0x03FF;

    // Check if the page table exists (the present bit, bit 0, is set).
    if (!(dir->tablesPhysical[pdindex] & 1)) {
        // Allocate a new physical block for the page table and clear it.
        uint32_t *new_pt_phys = (uint32_t *)pmm_alloc_block();
        memset(new_pt_phys, 0, 4096);
        
        // Add the new page table to the page directory.
        // 0x7 sets the present, writable, and user-accessible flags.
        dir->tablesPhysical[pdindex] = (uint32_t)new_pt_phys | 0x7;
    }

    // Get the physical address of the page table by masking out the flags.
    uint32_t  pt_phys = dir->tablesPhysical[pdindex] & 0xFFFFF000;
    uint32_t *pt_virt = (uint32_t *)pt_phys;

    // Set the page table entry to point to the physical frame with the requested flags.
    pt_virt[ptindex]  = ((uint32_t)phys) | I86_PTE_PRESENT | I86_PTE_WRITABLE | flags;

    // If modifying the currently active directory, flush the TLB so the CPU sees the change.
    if (dir == current_directory) {
        vmm_flush_tlb_entry(virt);
    }
}

// Maps a physical page to a virtual page in the currently active page directory.
void vmm_map_page(void *phys, void *virt, int flags) {
    vmm_map_page_in_dir(current_directory, phys, virt, flags);
}

// Creates a new address space for a process.
// It allocates a new page directory and links the kernel's page tables into it.
// This allows the kernel to be mapped into every process's address space.
page_directory_t *vmm_create_address_space() {
    page_directory_t *new_pd = (page_directory_t *)pmm_alloc_block();
    memset(new_pd, 0, sizeof(page_directory_t));

    // Copy the lower kernel mappings (typically used for basic hardware memory maps).
    for (int i = 0; i < 32; i++) {
        new_pd->tablesPhysical[i] = kernel_directory->tablesPhysical[i];
    }

    // Copy the higher half kernel mappings (0xC0000000 and above).
    // This allows the process to execute system calls and the kernel to run within the process context.
    for (int i = 768; i < 1024; i++) {
        new_pd->tablesPhysical[i] = kernel_directory->tablesPhysical[i];
    }

    return new_pd;
}

// Frees an address space and all its associated user-space physical frames.
void vmm_free_address_space(page_directory_t *pd) {
    // Start at index 32 to avoid freeing kernel structures.
    for (int i = 32; i < 1024; i++) {
        uint32_t entry = pd->tablesPhysical[i];
        
        // If the page table is present, iterate through its entries.
        if (entry & I86_PTE_PRESENT) {
            uint32_t *pt_phys = (uint32_t *)(entry & 0xFFFFF000);
            
            for (int j = 0; j < 1024; j++) {
                uint32_t pt_entry = pt_phys[j];
                
                // Only free frames that are present and marked as user-space.
                // We do not want to accidentally free kernel memory.
                if ((pt_entry & I86_PTE_PRESENT) && (pt_entry & I86_PTE_USER)) {
                    void *frame = (void *)(pt_entry & 0xFFFFF000);
                    pmm_free_block(frame);
                }
            }
            // Free the physical block used by the page table itself.
            pmm_free_block(pt_phys);
        }
    }
    // Free the page directory block.
    pmm_free_block(pd);
}

// Switches the active page directory by loading its address into CR3.
void vmm_switch_directory(page_directory_t *dir) {
    if (!dir)
        return;
    current_directory = dir;
    vmm_load_pd((uint32_t *)dir);
}

// Returns a pointer to the currently active page directory.
page_directory_t *vmm_get_current_directory() {
    return current_directory;
}

// Initializes the virtual memory manager.
// It creates the initial kernel page directory and identity maps the first 128MB.
void init_vmm() {
    kernel_directory = (page_directory_t *)pmm_alloc_block();
    memset(kernel_directory, 0, sizeof(page_directory_t));

    // Identity map the first 128MB of memory.
    // The virtual address matches the physical address (e.g., 0x1000 -> 0x1000).
    uint32_t i = 0;
    while (i < 128 * 1024 * 1024) {
        vmm_map_page_in_dir(kernel_directory, (void *)i, (void *)i,
                            I86_PTE_PRESENT | I86_PTE_WRITABLE);
        i += 4096;
    }

    // Activate the kernel page directory and enable paging.
    vmm_switch_directory(kernel_directory);
    serial_log(" [VMM] Identity Mapped 128MB (Supervisor Only).\n");
}