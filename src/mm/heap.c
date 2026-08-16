/*
 * Heap Memory Manager
 *
 * Implements a linked-list based memory allocator (kmalloc and kfree) for the kernel.
 * The heap provides dynamic memory allocation, allowing the kernel to request
 * arbitrary-sized blocks of memory at runtime.
 * 
 * Memory is mapped into the virtual address space during initialization,
 * starting at HEAP_START.
 */
#include "heap.h"

extern void *pmm_alloc_block();
extern void  vmm_map_page(void *phys, void *virt, int flags);
extern void  term_print(const char *str);

#define HEAP_START 0xD0000000
#define HEAP_SIZE (32 * 1024 * 1024) // 32MB
#define BLOCK_SIZE 4096

// Header prepended to every allocated and free block.
// It tracks the size and availability of the block, and links to the next block.
typedef struct alloc_header {
    struct alloc_header *next;
    uint32_t             size;    // Size of the usable memory area (excluding header)
    uint8_t              is_free; // 1 if free, 0 if allocated
} alloc_header_t;

alloc_header_t *free_list_head = 0;

// Initializes the kernel heap by allocating physical frames and mapping them
// into the virtual address space.
void init_heap() {
    void *heap_start = (void *)HEAP_START;

    // Allocate and map physical pages to back the entire heap size.
    for (uint32_t i = 0; i < HEAP_SIZE; i += BLOCK_SIZE) {
        void *phys = pmm_alloc_block();
        if (!phys) {
            term_print(" [HEAP] OOM during init!\n");
            return; // Initialization failed due to out-of-memory
        }
        // Map the physical frame to the virtual heap address with read/write permissions.
        vmm_map_page(phys, (void *)((uint32_t)heap_start + i), 0x3);
    }

    // Set up the initial free list block covering the entire heap.
    free_list_head          = (alloc_header_t *)heap_start;
    free_list_head->size    = HEAP_SIZE - sizeof(alloc_header_t);
    free_list_head->is_free = 1;
    free_list_head->next    = 0;
}

// Allocates a requested number of bytes from the heap.
// It uses a first-fit search algorithm to find an available block.
void *kmalloc(size_t size) {
    uint32_t eflags;
    // Disable interrupts to prevent concurrent access issues during allocation.
    __asm__ volatile("pushf; pop %0; cli" : "=r"(eflags));

    // Align the requested size to 4-byte boundaries to maintain memory alignment requirements.
    if (size % 4 != 0)
        size += (4 - (size % 4));

    alloc_header_t *current = free_list_head;

    while (current) {
        if (current->is_free && current->size >= size) {
            // If the block is significantly larger than needed, split it into two.
            // This leaves the remaining space available for future allocations.
            if (current->size > size + sizeof(alloc_header_t) + 4) {
                alloc_header_t *new_block =
                    (alloc_header_t *)((uint32_t)current + sizeof(alloc_header_t) + size);
                new_block->is_free = 1;
                new_block->size    = current->size - size - sizeof(alloc_header_t);
                new_block->next    = current->next;
                
                current->size      = size;
                current->next      = new_block;
            }
            
            // Mark the selected block as allocated.
            current->is_free = 0;
            
            // Restore previous interrupt state.
            __asm__ volatile("push %0; popf" : : "r"(eflags));
            
            // Return a pointer to the usable memory area (just past the header).
            return (void *)((uint32_t)current + sizeof(alloc_header_t));
        }
        current = current->next;
    }

    // Restore previous interrupt state on failure.
    __asm__ volatile("push %0; popf" : : "r"(eflags));
    return 0; // No suitable block found
}

// Scans the free list and merges adjacent free blocks.
// This prevents fragmentation by combining smaller contiguous free blocks into larger ones.
void heap_coalesce() {
    alloc_header_t *curr = free_list_head;
    while (curr && curr->next) {
        if (curr->is_free && curr->next->is_free) {
            // Merge the current block with the next one.
            curr->size += curr->next->size + sizeof(alloc_header_t);
            curr->next = curr->next->next;
            // Do not advance `curr` yet, as the new `next` block might also be free.
        } else {
            curr = curr->next;
        }
    }
}

// Frees a previously allocated memory block.
void kfree(void *ptr) {
    if (!ptr)
        return;

    uint32_t eflags;
    // Disable interrupts to ensure atomic updates to the heap structures.
    __asm__ volatile("pushf; pop %0; cli" : "=r"(eflags));

    // Retrieve the header preceding the allocated memory.
    alloc_header_t *header = (alloc_header_t *)((uint32_t)ptr - sizeof(alloc_header_t));
    
    // Mark the block as free.
    header->is_free = 1;
    
    // Attempt to merge the newly freed block with adjacent free blocks.
    heap_coalesce();

    // Restore previous interrupt state.
    __asm__ volatile("push %0; popf" : : "r"(eflags));
}