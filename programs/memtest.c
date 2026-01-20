#include "stdlib.h"

int main() {
    print("\n--- Memory Test ---\n");

    print("Allocating 1 KB...\n");
    char* buffer = (char*)malloc(1024);
    
    if (buffer == 0) {
        print("Malloc failed!\n");
        return 1;
    }
    
    // Write to it
    for(int i=0; i<1024; i++) buffer[i] = 'A';
    buffer[1023] = 0;
    
    print("Write success. Reading start: ");
    char temp[2] = {buffer[0], 0};
    print(temp); // Should be 'A'
    print("\n");

    print("Freeing...\n");
    free(buffer);

    print("Allocating 50 KB (Force sbrk expansion)...\n");
    // This will force the kernel to map NEW pages
    char* big_buf = (char*)malloc(50 * 1024);
    if(big_buf) print("Success!\n");
    else print("Failed!\n");

    print("-------------------\n");
    return 0;
}