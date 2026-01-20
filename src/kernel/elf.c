/* src/kernel/elf.c */
#include "elf.h"
#include "../mm/heap.h"
#include "../mm/vmm.h"
#include "process.h"

extern void term_print(const char* str);
extern void* pmm_alloc_block();
extern void set_cr3(uint32_t pd);
extern uint32_t get_cr3();

typedef struct file_node {
    char name[32];
    char* data;         
    uint32_t size;
    struct file_node* next; 
} file_t;
extern file_t* fs_resolve_path(const char* path);

extern page_directory_t* vmm_create_address_space();
extern void vmm_map_page_in_dir(page_directory_t* pd, void* phys, void* virt, int flags);

void* memcpy(void* dest, const void* src, uint32_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    while(n--) *d++ = *s++;
    return dest;
}

void* memset(void* ptr, int value, uint32_t num) {
    unsigned char* p = (unsigned char*)ptr;
    while(num--) *p++ = (unsigned char)value;
    return ptr;
}

int elf_load_file(const char* filename, char* args) {
    file_t* f = fs_resolve_path(filename);
    if (!f) {
        term_print("ELF: File not found.\n");
        return -1;
    }

    elf_header_t* hdr = (elf_header_t*)f->data;
    if (hdr->magic != ELF_MAGIC) {
        term_print("ELF: Not a valid ELF file.\n");
        return -1;
    }

    term_print("ELF: Loading "); term_print(filename); term_print("\n");

    page_directory_t* new_pd = vmm_create_address_space();
    if (!new_pd) return -1;

    uint32_t highest_addr = 0;
    uint32_t old_cr3 = get_cr3();
    
    __asm__ volatile("cli");
    set_cr3((uint32_t)new_pd); 

    elf_program_header_t* ph = (elf_program_header_t*)(f->data + hdr->phoff);
    for (int i = 0; i < hdr->phnum; i++) {
        if (ph[i].type == PT_LOAD) {
            uint32_t memsz = ph[i].memsz;
            uint32_t filesz = ph[i].filesz;
            uint32_t vaddr = ph[i].vaddr;
            uint32_t offset = ph[i].offset;

            uint32_t base_addr = vaddr & 0xFFFFF000;
            uint32_t end_addr = vaddr + memsz;
            uint32_t page_count = (end_addr - base_addr + 4095) / 4096;

            for (uint32_t z = 0; z < page_count; z++) {
                void* frame = pmm_alloc_block();
                vmm_map_page_in_dir(new_pd, frame, (void*)(base_addr + (z * 4096)), 0x7);
            }
            
            if (memsz > filesz) {
                memset((void*)(vaddr + filesz), 0, memsz - filesz);
            }
            memcpy((void*)vaddr, (void*)(f->data + offset), filesz);

            if (end_addr > highest_addr) highest_addr = end_addr;
        }
    }

    set_cr3(old_cr3);
    __asm__ volatile("sti");

    if (highest_addr % 4096 != 0) {
        highest_addr = (highest_addr & 0xFFFFF000) + 4096;
    }

    term_print("ELF: Executing...\n");
    
    // PASS 0 for is_kernel (User Mode)
    return create_process((void (*)())hdr->entry, args, highest_addr, 0);
}