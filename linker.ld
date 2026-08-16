/*
 * Kernel Linker Script
 * 
 * Defines the memory layout for the kernel executable. It places the kernel
 * starting at the 2MB mark (0x200000) and aligns sections to 4KB page boundaries.
 */

ENTRY(_start)
SECTIONS
{
    /* Set the load address to 2MB to leave lower memory free for hardware structures */
    . = 0x200000;
    
    /* Code section */
    .text ALIGN(4096) : {
        KEEP(*(.multiboot))
        *(.text)
    }
    
    /* Read-only data section */
    .rodata ALIGN(4096) : {
        *(.rodata*)
    }
    
    /* Initialized read/write data section */
    .data ALIGN(4096) : {
        *(.data)
    }
    
    /* Uninitialized data section (BSS) */
    .bss ALIGN(4096) : {
        *(COMMON)
        *(.bss)
    }
    
    /* Discard unnecessary ELF sections to reduce binary size */
    /DISCARD/ : {
        *(.note.gnu.build-id)
        *(.comment)
        *(.note*)
    }
}
