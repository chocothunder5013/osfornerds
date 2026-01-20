/* src/mm/vmm.c */
#include "vmm.h"

extern void* pmm_alloc_block();
extern void pmm_free_block(void* p);
extern void serial_log(char *str);
extern void* memset(void* ptr, int value, uint32_t num); 

page_directory_t* current_directory = 0;
page_directory_t* kernel_directory = 0;

void vmm_flush_tlb_entry(void* addr) {
   __asm__ volatile("invlpg (%0)" ::"r" (addr) : "memory");
}

void vmm_load_pd(uint32_t* addr) {
    __asm__ volatile("mov %0, %%cr3" :: "r"(addr));
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // Enable Paging
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0));
}

uint32_t get_cr3() {
    uint32_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

void set_cr3(uint32_t pd) {
    __asm__ volatile("mov %0, %%cr3" :: "r"(pd));
}

void vmm_map_page_in_dir(page_directory_t* dir, void* phys, void* virt, int flags) {
    uint32_t pdindex = (uint32_t)virt >> 22;
    uint32_t ptindex = ((uint32_t)virt >> 12) & 0x03FF;

    if (!(dir->tablesPhysical[pdindex] & I86_PTE_PRESENT)) {
        uint32_t* new_pt_phys = (uint32_t*)pmm_alloc_block();
        memset(new_pt_phys, 0, 4096); 
        dir->tablesPhysical[pdindex] = (uint32_t)new_pt_phys | I86_PTE_PRESENT | I86_PTE_WRITABLE | I86_PTE_USER;
    }

    uint32_t pt_phys = dir->tablesPhysical[pdindex] & 0xFFFFF000;
    uint32_t* pt_virt = (uint32_t*)pt_phys; // NOTE: In a real higher-half kernel, we would need to map this temporarily.
                                            // Since we identity map low memory, this works for now.

    pt_virt[ptindex] = ((uint32_t)phys) | I86_PTE_PRESENT | I86_PTE_WRITABLE | flags;
    
    if (dir == current_directory) {
        vmm_flush_tlb_entry(virt);
    }
}

void vmm_map_page(void* phys, void* virt, int flags) {
    vmm_map_page_in_dir(current_directory, phys, virt, flags);
}

page_directory_t* vmm_create_address_space() {
    page_directory_t* new_pd = (page_directory_t*)pmm_alloc_block();
    memset(new_pd, 0, sizeof(page_directory_t));
    new_pd->physicalAddr = (uint32_t)new_pd;
    
    // Copy Kernel Identity Mappings (0-128MB)
    // Critical: We share kernel tables, we do NOT copy the pages themselves, just the pointers to tables.
    for (int i = 0; i < 32; i++) { 
         new_pd->tablesPhysical[i] = kernel_directory->tablesPhysical[i];
    }
    return new_pd;
}

// --- FIX: Added Cleanup Function ---
void vmm_free_address_space(page_directory_t* pd) {
    // 1. Loop through all Page Directory Entries
    // Start at 32 to skip Kernel Identity map (0-128MB)
    for (int i = 32; i < 1024; i++) {
        uint32_t entry = pd->tablesPhysical[i];
        
        if (entry & I86_PTE_PRESENT) {
            uint32_t* pt_phys = (uint32_t*)(entry & 0xFFFFF000);
            
            // 2. Loop through Page Table Entries
            for (int j = 0; j < 1024; j++) {
                 uint32_t pt_entry = pt_phys[j];
                 
                 // Only free if Present and NOT a kernel page (sanity check)
                 if ((pt_entry & I86_PTE_PRESENT) && (pt_entry & I86_PTE_USER)) {
                     void* frame = (void*)(pt_entry & 0xFFFFF000);
                     pmm_free_block(frame);
                 }
            }
            
            // 3. Free the Page Table itself
            pmm_free_block(pt_phys);
        }
    }
    // 4. Free the Directory itself
    pmm_free_block(pd);
}

void vmm_switch_directory(page_directory_t* dir) {
    if (!dir) return;
    current_directory = dir;
    vmm_load_pd((uint32_t*)dir->physicalAddr);
}

page_directory_t* vmm_get_current_directory() {
    return current_directory;
}

void init_vmm() {
    kernel_directory = (page_directory_t*)pmm_alloc_block();
    memset(kernel_directory, 0, sizeof(page_directory_t));
    kernel_directory->physicalAddr = (uint32_t)kernel_directory;

    // FIX: Map 0-128MB as SUPERVISOR only (Remove I86_PTE_USER).
    // This prevents Ring 3 processes from touching kernel memory.
    uint32_t i = 0;
    while (i < 128 * 1024 * 1024) {
        // Flags: Present | Writable (0x3). NO User bit (0x4).
        vmm_map_page_in_dir(kernel_directory, (void*)i, (void*)i, I86_PTE_PRESENT | I86_PTE_WRITABLE);
        i += 4096;
    }

    vmm_switch_directory(kernel_directory);
    serial_log(" [VMM] Identity Mapped 128MB (Supervisor Only).\n");
}