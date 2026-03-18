; programs/entry.S
global _start
extern main
extern exit

_start:
    ; Stack is already set up by the kernel
    
    ; Push argc, argv (We don't support them yet, push 0)
    push 0 ; argv
    push 0 ; argc
    
    call main
    
    ; CLEANUP: Remove argc and argv from the stack (2 * 4 bytes = 8 bytes)
    add esp, 8
    
    ; If main returns, call exit with its return value (stored in eax)
    push eax
    call exit
    hlt

; Tell the linker this object does NOT require an executable stack
section .note.GNU-stack noalloc noexec nowrite progbits