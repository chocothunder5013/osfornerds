/* linker.ld */
ENTRY(_start)

SECTIONS
{
    . = 0x200000;

    .text : {
        *(.multiboot)   /* <--- This MUST be the first line inside .text */
        *(.text)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(COMMON)
        *(.bss)
    }
}