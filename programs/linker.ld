/*
 * Userland Linker Script
 * 
 * Defines the memory layout for userland executables. It places the program
 * starting at the 256MB mark (0x10000000), separating user space from kernel space.
 */

ENTRY(_start)
SECTIONS
{
    /* Set the load address to 256MB */
    . = 0x10000000;
    
    /* Code section */
    .text : { *(.text) }
    
    /* Initialized data section */
    .data : { *(.data) }
    
    /* Uninitialized data section (BSS) */
    .bss  : { *(.bss)  }
}
