; src/cpu/isr_asm.S
global isr_handler_32  ; Timer (IRQ0) is remapped to 32
global isr_handler_33  ; Keyboard (IRQ1) is remapped to 33
extern isr_handler     ; The C function we will call

; Macro for defining interrupt stubs
%macro ISR_NOERRCODE 1  ; For interrupts without error codes
    global isr%1
    isr%1:
        cli             ; Disable interrupts
        push 0          ; Push dummy error code
        push %1         ; Push interrupt number
        jmp isr_common_stub
%endmacro

; Define specific handlers we care about
ISR_NOERRCODE 32
ISR_NOERRCODE 33
ISR_NOERRCODE 44
ISR_NOERRCODE 128

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