; src/kernel/boot.S
MBALIGN  equ  1 << 0
MEMINFO  equ  1 << 1
VIDEO    equ  1 << 2  ; <--- NEW: Request Video Mode
FLAGS    equ  MBALIGN | MEMINFO | VIDEO
MAGIC    equ  0x1BADB002
CHECKSUM equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM
    dd 0, 0, 0, 0, 0 ; Header Address fields (unused if not generic)
    
    ; --- NEW: Video Request Fields ---
    dd 0             ; Mode Type (0 = Linear Graphics)
    dd 1024          ; Width
    dd 768           ; Height
    dd 32            ; Depth (Bits per pixel)

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

section .text
global _start
extern kmain

_start:
    mov esp, stack_top
    push ebx ; Push Multiboot Info Struct pointer
    cli
    call kmain
.hang:
    hlt
    jmp .hang