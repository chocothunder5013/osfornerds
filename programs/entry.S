    ; Userland Application Runtime Entry Point
    ; Initializes the standard C argument frame, transfers control to main(), and invokes sys_exit upon return.
    ; This code runs first when an ELF executable is loaded by the OS.
global _start
extern main
extern exit
_start:
    ; Push initial NULL placeholders for argv and argc.
    ; These prepare the stack for standard C main(int argc, char **argv) signatures.
    push 0
    push 0
    call main
    ; Clean up the 8-byte argument frame from the stack.
    add esp, 8
    ; Pass the return code from main (stored in EAX) to exit().
    push eax
    call exit
    hlt
    ; Non-executable stack directive for ELF safety.
section .note.GNU-stack noalloc noexec nowrite progbits
