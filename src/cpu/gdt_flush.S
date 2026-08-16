    ; GDT & TSS Register Flush Assembly Routine
    ; Reloads GDTR, updates segment registers (DS, ES, FS, GS, SS), flushes CS via far jump, and loads TR.
global gdt_flush
global tss_flush

    ; Reloads GDTR and updates segment selectors to kernel data (0x10) and code (0x08).
    ; This must be done in assembly because the CPU's segment registers cannot be directly
    ; manipulated using standard C code, and the CS register requires a far jump to update.
gdt_flush:
    mov eax, [esp+4]    ; Load GDT pointer argument from stack
    lgdt [eax]          ; Reload GDTR register with the new table

    ; Update all data segment registers to use the Kernel Data Segment (offset 0x10 in the GDT).
    mov ax, 0x10        
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Perform a far jump to update the Code Segment (CS) register.
    ; This flushes the CPU instruction pipeline and sets CS to the Kernel Code Segment (offset 0x08).
    jmp 0x08:.flush     
.flush:
    ret

    ; Loads the Task Register (TR) with the Task State Segment (TSS) descriptor.
    ; This tells the CPU where to find the kernel stack when transitioning from user mode to kernel mode.
tss_flush:
    mov ax, 0x28        ; Selector 0x28 points to the 5th entry in the GDT (the TSS).
    ltr ax              ; Load Task Register
    ret
