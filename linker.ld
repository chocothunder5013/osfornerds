ENTRY(_start)

SECTIONS
{
    . = 0x200000;

    .text ALIGN(4096) : {
        KEEP(*(.multiboot))   /* <--- Added KEEP to protect it */
        *(.text)
    }

    .rodata ALIGN(4096) : {
        *(.rodata*)
    }

    .data ALIGN(4096) : {
        *(.data)
    }

    .bss ALIGN(4096) : {
        *(COMMON)
        *(.bss)
    }

    /* Throw away modern GCC junk that pushes our headers too far down */
    /DISCARD/ : {
        *(.note.gnu.build-id)
        *(.comment)
        *(.note*)
    }
}
