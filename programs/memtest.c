#include "stdlib.h"

/*
 * memtest program
 * 
 * Verifies the userland memory allocator (malloc/free) by allocating
 * and writing to memory blocks. Also tests heap expansion by requesting
 * a block larger than the initial heap size.
 */

// Entry point for the memory allocation test program.
int main() {
    print("
--- Memory Test ---
");
    print("Allocating 1 KB...
");
    
    // Attempt to allocate 1 KB of memory.
    char *buffer = (char *)malloc(1024);
    if (buffer == 0) {
        print("Malloc failed!
");
        return 1;
    }
    
    // Write characters to the allocated buffer to verify the memory is mapped and writable.
    for (int i = 0; i < 1024; i++)
        buffer[i] = 'A';
    buffer[1023] = 0;
    
    print("Write success. Reading start: ");
    char temp[2] = {buffer[0], 0};
    print(temp);
    print("
");
    print("Freeing...
");
    
    // Release the memory block so it can be reused.
    free(buffer);
    
    print("Allocating 50 KB (Force sbrk expansion)...
");
    
    // Attempt a larger allocation. If the requested size exceeds the current
    // heap limit, the allocator calls sbrk() to request more memory from the kernel.
    char *big_buf = (char *)malloc(50 * 1024);
    if (big_buf)
        print("Success!
");
    else
        print("Failed!
");
        
    print("-------------------
");
    return 0;
}
