/* src/mm/pmm.c */
#include <stdint.h>
#include <stddef.h>

#define BLOCK_SIZE 4096

// Bitmap to track memory usage (1 = Used, 0 = Free)
// 32768 bytes * 8 bits * 4096 bytes = 1GB addressable
uint8_t memory_bitmap[32768]; 
uint32_t used_blocks = 0;
uint32_t max_blocks = 0;

void pmm_set(uint32_t bit) {
    memory_bitmap[bit / 8] |= (1 << (bit % 8));
}

void pmm_unset(uint32_t bit) {
    memory_bitmap[bit / 8] &= ~(1 << (bit % 8));
}

int pmm_test(uint32_t bit) {
    return memory_bitmap[bit / 8] & (1 << (bit % 8));
}

int pmm_find_first_free() {
    for (uint32_t i = 0; i < max_blocks / 8; i++) {
        if (memory_bitmap[i] != 0xFF) { 
            for (int j = 0; j < 8; j++) {
                int bit = 1 << j;
                if (!(memory_bitmap[i] & bit)) return i * 8 + j;
            }
        }
    }
    return -1;
}

void init_pmm(uint32_t mem_size_kb) {
    max_blocks = (mem_size_kb * 1024) / BLOCK_SIZE;
    used_blocks = max_blocks;

    // 1. Mark ALL memory as used initially (Safety First)
    for(uint32_t i=0; i < max_blocks / 8; i++) memory_bitmap[i] = 0xFF;

    // 2. Free the usable RAM (Start at 4MB to protect Kernel/Modules)
    // We assume contiguous RAM for QEMU.
    uint32_t mem_start_block = (4 * 1024 * 1024) / BLOCK_SIZE;
    
    for (uint32_t i = mem_start_block; i < max_blocks; i++) {
        pmm_unset(i);
        used_blocks--;
    }
}

void* pmm_alloc_block() {
    if (max_blocks - used_blocks <= 0) return 0;
    
    int frame = pmm_find_first_free();
    if (frame == -1) return 0;

    pmm_set(frame);
    used_blocks++;
    
    return (void*)(frame * BLOCK_SIZE);
}

void pmm_free_block(void* p) {
    uint32_t addr = (uint32_t)p;
    uint32_t frame = addr / BLOCK_SIZE;
    
    if (frame >= max_blocks) return;
    
    pmm_unset(frame);
    used_blocks--;
}