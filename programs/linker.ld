ENTRY(_start)
SECTIONS
{
    /* Was 0x400000, changed to 256MB to avoid Kernel collision */
    . = 0x10000000; 
    
    .text : { *(.text) }
    .data : { *(.data) }
    .bss  : { *(.bss)  }
}