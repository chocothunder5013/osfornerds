    ; Low-Level Interrupt Assembly Stubs and Context Switching
    ; Defines ISR entry points, standard register saves/restores, C handler dispatching, and task context switching.
extern isr_handler

    ; Macro for interrupts without a CPU error code.
    ; Pushes a dummy 0 to ensure the stack frame aligns perfectly with the registers_t structure
    ; expected by the C handler, maintaining a consistent layout regardless of the interrupt type.
    %macro ISR_NOERRCODE 1
global isr%1
isr%1:
    cli                 ; Disable interrupts to prevent nested faults
    push 0              ; Dummy error code
    push %1             ; Push the interrupt number
    jmp isr_common_stub
    %endmacro

    ; Macro for interrupts with a CPU error code.
    ; The CPU automatically pushes the error code onto the stack before jumping here,
    ; so we only need to push the interrupt number.
    %macro ISR_ERRCODE 1
global isr%1
isr%1:
    cli                 ; Disable interrupts
    push %1             ; Push the interrupt number (error code is already on the stack)
    jmp isr_common_stub
    %endmacro

    ; CPU Exception Gate Vectors (0-31)
    ISR_NOERRCODE 0   ; Divide-by-zero
    ISR_NOERRCODE 1   ; Debug exception
    ISR_NOERRCODE 2   ; Non-maskable interrupt
    ISR_NOERRCODE 3   ; Breakpoint
    ISR_NOERRCODE 4   ; Overflow
    ISR_NOERRCODE 5   ; Bound range exceeded
    ISR_NOERRCODE 6   ; Invalid opcode
    ISR_NOERRCODE 7   ; Device not available
    ISR_ERRCODE   8   ; Double fault
    ISR_NOERRCODE 9   ; Coprocessor segment overrun
    ISR_ERRCODE   10  ; Invalid TSS
    ISR_ERRCODE   11  ; Segment not present
    ISR_ERRCODE   12  ; Stack-segment fault
    ISR_ERRCODE   13  ; General protection fault
    ISR_ERRCODE   14  ; Page fault
    ISR_NOERRCODE 15  ; Reserved
    ISR_NOERRCODE 16  ; x87 Floating-point exception
    ISR_ERRCODE   17  ; Alignment check
    ISR_NOERRCODE 18  ; Machine check
    ISR_NOERRCODE 19  ; SIMD Floating-point exception
    ISR_NOERRCODE 20  ; Virtualization exception

    ; Hardware IRQ and System Call Vectors
    ISR_NOERRCODE 32  ; IRQ0: Timer
    ISR_NOERRCODE 33  ; IRQ1: Keyboard
    ISR_NOERRCODE 44  ; IRQ12: Mouse
    ISR_NOERRCODE 128 ; INT 0x80: System call
    ISR_NOERRCODE 46  ; IRQ14: Primary ATA
    ISR_NOERRCODE 47  ; IRQ15: Secondary ATA

    ; Common assembly trampoline: saves registers, loads kernel data segment, invokes C handler, and restores state.
    ; This standardizes the environment before executing higher-level kernel code.
isr_common_stub:
    pusha               ; Push edi, esi, ebp, esp, ebx, edx, ecx, eax

    ; Save the original data segment and switch to the kernel data segment (0x10)
    mov ax, ds
    push eax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Call the C interrupt handler, passing a pointer to the saved registers.
    push esp            ; Pass pointer to registers_t struct on stack
    call isr_handler
    add esp, 4          ; Clean up the pushed pointer

    ; Restore the original data segment
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa                ; Restore general purpose registers
    add esp, 8          ; Clean up pushed interrupt vector and error code
    sti                 ; Re-enable interrupts
    iret                ; Return from interrupt (restores CS, EIP, EFLAGS, SS, and ESP)

    ; void switch_task(uint32_t *old_esp_ptr, uint32_t new_esp);
    ; Saves register context of running process, updates stack pointer, and restores new process registers.
    ; This is the core of the kernel's multitasking capability.
global switch_task
switch_task:
    mov eax, [esp+4]    ; Load pointer to old process ESP variable
    mov edx, [esp+8]    ; Load new process ESP value

    ; Push state that the C compiler does not automatically save
    pushf
    push cs
    push dword 0        ; Dummy placeholder for stack frame alignment
    pusha
    push ds
    push es
    push fs
    push gs

    ; Perform the actual stack switch
    mov [eax], esp      ; Store current stack pointer into old_esp_ptr
    mov esp, edx        ; Switch active stack pointer to new_esp

    ; Pop state for the new task
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 4          ; Skip dummy placeholder
    add esp, 4          ; Skip CS
    popf

    ret

    ; Trampoline to execute IRET for newly initialized process contexts.
    ; Used when starting a task for the first time to drop it into user space (Ring 3).
global jump_to_user
jump_to_user:
    iret
