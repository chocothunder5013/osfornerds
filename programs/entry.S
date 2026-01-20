; programs/entry.S
global _start
extern main
extern exit

_start:
    ; Stack is already set up by the kernel at 0xBFFFF000
    
    ; Push argc, argv (We don't support them yet, push 0)
    push 0 ; argv
    push 0 ; argc
    
    call main
    
    ; If main returns, call exit
    push eax
    call exit
    hlt