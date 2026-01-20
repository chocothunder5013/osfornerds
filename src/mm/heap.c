/* src/mm/heap.c */
#include "heap.h"

extern void* pmm_alloc_block();
extern void vmm_map_page(void* phys, void* virt, int flags); 
extern void term_print(const char* str);

#define HEAP_START 0xD0000000
#define HEAP_SIZE  (16 * 1024 * 1024)
#define BLOCK_SIZE 4096

typedef struct alloc_header {
    struct alloc_header* next;
    uint32_t size;  
    uint8_t is_free;
} alloc_header_t;

alloc_header_t* free_list_head = 0;

void init_heap() {
    void* heap_start = (void*)HEAP_START;
    for (uint32_t i = 0; i < HEAP_SIZE; i += BLOCK_SIZE) {
        void* phys = pmm_alloc_block();
        if (!phys) {
            term_print(" [HEAP] OOM during init!\n");
            return;
        }
        vmm_map_page(phys, (void*)((uint32_t)heap_start + i), 0x3);
    }

    free_list_head = (alloc_header_t*)heap_start;
    free_list_head->size = HEAP_SIZE - sizeof(alloc_header_t);
    free_list_head->is_free = 1;
    free_list_head->next = 0;
}

void* kmalloc(size_t size) {
    uint32_t eflags;
    // Save interrupt state and disable
    __asm__ volatile("pushf; pop %0; cli" : "=r"(eflags));

    if (size % 4 != 0) size += (4 - (size % 4));
    alloc_header_t* current = free_list_head;

    while (current) {
        if (current->is_free && current->size >= size) {
            if (current->size > size + sizeof(alloc_header_t) + 4) {
                alloc_header_t* new_block = (alloc_header_t*)((uint32_t)current + sizeof(alloc_header_t) + size);
                new_block->is_free = 1;
                new_block->size = current->size - size - sizeof(alloc_header_t);
                new_block->next = current->next;
                current->size = size;
                current->next = new_block;
            }
            current->is_free = 0;
            
            // Restore interrupt state
            __asm__ volatile("push %0; popf" : : "r"(eflags));
            return (void*)((uint32_t)current + sizeof(alloc_header_t));
        }
        current = current->next;
    }
    
    // Restore interrupt state
    __asm__ volatile("push %0; popf" : : "r"(eflags));
    return 0;
}

void heap_coalesce() {
    alloc_header_t* curr = free_list_head;
    while (curr && curr->next) {
        if (curr->is_free && curr->next->is_free) {
            curr->size += curr->next->size + sizeof(alloc_header_t);
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}

void kfree(void* ptr) {
    if (!ptr) return;
    uint32_t eflags;
    // Save interrupt state and disable
    __asm__ volatile("pushf; pop %0; cli" : "=r"(eflags));
    
    alloc_header_t* header = (alloc_header_t*)((uint32_t)ptr - sizeof(alloc_header_t));
    header->is_free = 1;
    heap_coalesce();

    // Restore interrupt state
    __asm__ volatile("push %0; popf" : : "r"(eflags));
}