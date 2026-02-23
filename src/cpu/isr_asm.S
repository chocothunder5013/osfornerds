; src/cpu/isr_asm.S

extern isr_handler

; 1. Define ISRs that DO NOT push an error code (e.g., Timer, Div by Zero)
; We push a dummy '0' so the stack layout is consistent.
%macro ISR_NOERRCODE 1
    global isr%1
    isr%1:
        cli
        push 0          ; Push dummy error code
        push %1         ; Push interrupt number
        jmp isr_common_stub
%endmacro

; 2. Define ISRs that DO push an error code (e.g., Page Fault, GPF)
; The CPU has already pushed the error code, so we just push the INT number.
%macro ISR_ERRCODE 1
    global isr%1
    isr%1:
        cli
        ; Error code is already on stack!
        push %1         ; Push interrupt number
        jmp isr_common_stub
%endmacro

; --- Define the Exception Handlers (0-31) ---
ISR_NOERRCODE 0   ; Divide by Zero
ISR_NOERRCODE 1   ; Debug
ISR_NOERRCODE 2   ; NMI
ISR_NOERRCODE 3   ; Breakpoint
ISR_NOERRCODE 4   ; Overflow
ISR_NOERRCODE 5   ; Bound Range Exceeded
ISR_NOERRCODE 6   ; Invalid Opcode
ISR_NOERRCODE 7   ; Device Not Available
ISR_ERRCODE   8   ; Double Fault (Has Error Code!)
ISR_NOERRCODE 9   ; Coprocessor Segment Overrun
ISR_ERRCODE   10  ; Invalid TSS
ISR_ERRCODE   11  ; Segment Not Present
ISR_ERRCODE   12  ; Stack-Segment Fault
ISR_ERRCODE   13  ; General Protection Fault
ISR_ERRCODE   14  ; Page Fault (Has Error Code!)
ISR_NOERRCODE 15  ; Reserved
ISR_NOERRCODE 16  ; x87 Floating-Point Exception
ISR_ERRCODE   17  ; Alignment Check
ISR_NOERRCODE 18  ; Machine Check
ISR_NOERRCODE 19  ; SIMD Floating-Point Exception
ISR_NOERRCODE 20  ; Virtualization Exception

; Define specific handlers we care about
ISR_NOERRCODE 32
ISR_NOERRCODE 33
ISR_NOERRCODE 44
ISR_NOERRCODE 128
ISR_NOERRCODE 46  ; IRQ 14 (Primary IDE)
ISR_NOERRCODE 47  ; IRQ 15 (Secondary IDE)

isr_common_stub:
    pusha               ; Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax

    mov ax, ds          ; Lower 16-bits of eax = ds.
    push eax            ; save the data segment descriptor

    mov ax, 0x10        ; load the kernel data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; --- FIX START ---
    push esp            ; 1. Push the POINTER to the stack (registers_t*)
    call isr_handler    ; 2. Call the C function
    add esp, 4          ; 3. Clean up the pointer we pushed
    ; --- FIX END ---

    pop eax             ; reload the original data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa                ; Pop edi,esi,ebp...
    add esp, 8          ; Cleans up the pushed error code and ISR number
    sti                 ; Re-enable interrupts
    iret                ; return from interrupt

; void switch_task(uint32_t *old_esp_ptr, uint32_t new_esp);
global switch_task
switch_task:
    ; 1. Save the context of the OLD process
    ; The C calling convention puts return addr at [esp], arg1 at [esp+4]
    
    mov eax, [esp+4]    ; Get pointer to old_esp_ptr (arg1)
    mov edx, [esp+8]    ; Get new_esp (arg2)

    pushf               ; Save EFLAGS
    push cs             ; Save CS (conceptually, though we are in kernel)
    push dword 0             ; Dummy EIP placeholder (not strictly needed here but keeps alignment)
    
    pusha               ; Save general purpose regs (EAX, ECX...)
    
    push ds
    push es
    push fs
    push gs

    ; Save the current ESP into the pointer provided (old_esp_ptr)
    mov [eax], esp

    ; 2. Load the context of the NEW process
    mov esp, edx        ; SWITCH STACKS!

    ; 3. Restore registers from the new stack
    pop gs
    pop fs
    pop es
    pop ds
    
    popa
    
    add esp, 4          ; Skip dummy EIP/Error code
    add esp, 4          ; Skip CS
    popf                ; Restore EFLAGS

    ret                 ; Returns to the EIP saved on the NEW stack!

; Helper to pop the IRET frame for new processes
global jump_to_user
jump_to_user:
    iret