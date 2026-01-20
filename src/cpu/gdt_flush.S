; src/cpu/gdt_flush.S
global gdt_flush
global tss_flush

; ---------------------------------------------------
; gdt_flush(uint32_t gdt_ptr)
; Reloads the GDT and segment registers.
; EXPECTS: 
;   GDT Entry 1 (0x08) = Kernel Code
;   GDT Entry 2 (0x10) = Kernel Data
; ---------------------------------------------------
gdt_flush:
    mov eax, [esp+4]    ; Get the pointer argument
    lgdt [eax]          ; Load the GDT Register

    mov ax, 0x10        ; Offset 0x10 (Entry 2) -> Kernel Data Segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Perform a Far Jump to flush the pipeline and reload CS
    jmp 0x08:.flush     ; Offset 0x08 (Entry 1) -> Kernel Code Segment
.flush:
    ret

; ---------------------------------------------------
; tss_flush()
; Loads the Task Register (TR). 
; Required for User Mode context switching.
; EXPECTS:
;   GDT Entry 5 (0x28) = TSS Entry
; ---------------------------------------------------
tss_flush:
    mov ax, 0x28        ; Index 5 * 8 bytes = 40 = 0x28
    ltr ax              ; Load Task Register
    ret