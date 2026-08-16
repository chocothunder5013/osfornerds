    ; Kernel Assembly Entry Point & Multiboot Specification Header
    ;
    ; This file defines the entry point for the kernel when loaded by a
    ; Multiboot1-compliant bootloader (such as GRUB). It sets up the magic
    ; numbers required for the bootloader to recognize the kernel, requests
    ; a specific video mode (1920x1080x32), allocates the initial stack,
    ; and transitions execution to the C kernel entry point (kmain).

MBALIGN  equ  1 << 0
MEMINFO  equ  1 << 1
VIDEO    equ  1 << 2
FLAGS    equ  MBALIGN | MEMINFO | VIDEO
MAGIC    equ  0x1BADB002
CHECKSUM equ -(MAGIC + FLAGS)

    ; Multiboot Header Section
    ; The bootloader searches for this signature within the first 8KB of the kernel file.
    ; It must be 32-bit aligned.
section .multiboot
    align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM
    dd 0, 0, 0, 0, 0 ; Unused physical address fields

    ; VBE Video Mode Specification Fields
    ; These fields request the bootloader to set up a linear framebuffer
    ; before handing control to the kernel.
    dd 0             ; Graphics mode type (0 = Linear graphics framebuffer)
    dd 1920          ; Display resolution width
    dd 1080          ; Display resolution height
    dd 32            ; Color depth (32 bits per pixel)

    ; Kernel Initial Stack Allocation
    ; A 16KB uninitialized data section (BSS) reserved for the kernel's initial call stack.
section .bss
    align 16
stack_bottom:
    resb 16384
stack_top:

    ; Boot Entry Point Called by Bootloader
section .text
global _start
extern kmain

_start:
    ; Initialize the stack pointer to the top of the allocated stack area.
    mov esp, stack_top

    ; The bootloader passes the Multiboot information structure pointer in EBX
    ; and the Multiboot magic value in EAX. Push these onto the stack as
    ; arguments for kmain(multiboot_info_t*, uint32_t).
    ; We subtract 8 from ESP first to maintain 16-byte stack alignment.
    sub esp, 8
    push eax
    push ebx
    cli
    call kmain

    ; If kmain returns, enter an infinite halt loop.
.hang:
    hlt
    jmp .hang
