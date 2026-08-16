#include "stdlib.h"

/*
 * crash program
 * 
 * Intentionally causes a fault to test the OS's exception handling
 * and process termination logic.
 */

// Entry point for the crash program.
int main() {
    print("
[CRASH] Initiating controlled userland crash...
");
    
    // Dereference a null pointer to trigger a page fault.
    // The operating system should catch this and terminate the process
    // without bringing down the entire system.
    volatile int *bad_ptr = (int *)0x00000000;
    *bad_ptr              = 42;
    
    print("If you see this, the OS failed to catch the crash!
");
    return 0;
}
