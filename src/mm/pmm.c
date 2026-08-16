/*
 * Physical Memory Manager (PMM)
 *
 * The PMM tracks available physical memory frames using a bitmap.
 * Each bit corresponds to a 4KB block of memory. A bit set to 1 means
 * the frame is allocated, while 0 means it is free.
 * 
 * The system memory map provided by the bootloader (like GRUB) typically
 * tells us how much memory exists. For simplicity, this implementation
 * assumes a contiguous memory space and reserves the first 4MB for the kernel.
 */
#include <stdint.h>
#include <stddef.h>

#define BLOCK_SIZE 4096

// Bitmap to track memory blocks. 32768 bytes * 8 bits = 262144 blocks.
// 262144 blocks * 4KB = 1GB of trackable memory.
uint8_t  memory_bitmap[32768];
uint32_t used_blocks = 0;
uint32_t max_blocks  = 0;

// Sets a specific bit in the bitmap to indicate the block is in use.
void pmm_set(uint32_t bit) {
    memory_bitmap[bit / 8] |= (1 << (bit % 8));
}

// Clears a specific bit in the bitmap to indicate the block is free.
void pmm_unset(uint32_t bit) {
    memory_bitmap[bit / 8] &= ~(1 << (bit % 8));
}

// Checks the state of a specific bit in the bitmap.
int pmm_test(uint32_t bit) {
    return memory_bitmap[bit / 8] & (1 << (bit % 8));
}

// Scans the bitmap to find the first free memory block.
// Returns the index of the first free block, or -1 if memory is full.
int pmm_find_first_free() {
    for (uint32_t i = 0; i < max_blocks / 8; i++) {
        // Skip fully allocated bytes to speed up the search.
        if (memory_bitmap[i] != 0xFF) {
            // Check individual bits within the byte.
            for (int j = 0; j < 8; j++) {
                int bit = 1 << j;
                if (!(memory_bitmap[i] & bit))
                    return i * 8 + j;
            }
        }
    }
    return -1;
}

// Initializes the physical memory manager.
// Marks all memory as used initially, then frees the usable regions.
// The lower 4MB is reserved to prevent the kernel code and data from being overwritten.
void init_pmm(uint32_t mem_size_kb) {
    max_blocks  = (mem_size_kb * 1024) / BLOCK_SIZE;
    used_blocks = max_blocks;

    // Set all memory as allocated by default.
    for (uint32_t i = 0; i < max_blocks / 8; i++)
        memory_bitmap[i] = 0xFF;

    // Free the blocks starting after the reserved 4MB kernel area.
    uint32_t mem_start_block = (4 * 1024 * 1024) / BLOCK_SIZE;
    for (uint32_t i = mem_start_block; i < max_blocks; i++) {
        pmm_unset(i);
        used_blocks--;
    }
}

// Allocates a single contiguous 4KB block of physical memory.
// Returns the physical address of the allocated block, or 0 if out of memory.
void *pmm_alloc_block() {
    if (max_blocks - used_blocks <= 0)
        return 0; // Out of memory

    int frame = pmm_find_first_free();
    if (frame == -1)
        return 0; // Double check against allocation failure

    pmm_set(frame);
    used_blocks++;

    // Convert frame index back to a physical address.
    return (void *)(frame * BLOCK_SIZE);
}

// Frees a previously allocated 4KB block of physical memory.
void pmm_free_block(void *p) {
    uint32_t addr  = (uint32_t)p;
    uint32_t frame = addr / BLOCK_SIZE;

    // Prevent out-of-bounds memory access if an invalid pointer is passed.
    if (frame >= max_blocks)
        return;

    pmm_unset(frame);
    used_blocks--;
}