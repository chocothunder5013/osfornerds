/* src/cpu/idt.c */
#include "idt.h"
#include "../drivers/serial.h" 
#include "../kernel/syscall.h" 

extern void isr128(); // Syscall
extern void isr32();  // Timer
extern void isr33();  // Keyboard
// NEW: Extern declaration for Mouse Interrupt Stub (must be in isr_asm.S)
extern void isr44();  

idt_entry_t idt[256];
idt_register_t idt_reg;

extern void keyboard_handler();
extern void schedule();
extern void mouse_handler();

void set_idt_gate(int n, uint32_t handler) {
    idt[n].base_low = handler & 0xFFFF;
    idt[n].sel = 0x08; 
    idt[n].always0 = 0;
    idt[n].flags = 0x8E; 
    idt[n].base_high = (handler >> 16) & 0xFFFF;
}

void remap_pic() {
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28); // Slave start 40 (0x28)
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);
    outb(0x21, 0x00); outb(0xA1, 0x00);
}

void init_idt() {
    idt_reg.base = (uint32_t) &idt;
    idt_reg.limit = 256 * sizeof(idt_entry_t) - 1;

    remap_pic();

    set_idt_gate(32, (uint32_t)isr32);
    set_idt_gate(33, (uint32_t)isr33);
    
    // FIX: Map Mouse (IRQ 12 -> Int 44)
    set_idt_gate(44, (uint32_t)isr44);

    // Syscall
    uint32_t handler = (uint32_t)isr128;
    idt[128].base_low = handler & 0xFFFF;
    idt[128].sel = 0x08;
    idt[128].always0 = 0;
    idt[128].flags = 0xEE; 
    idt[128].base_high = (handler >> 16) & 0xFFFF;

    __asm__ volatile("lidt (%0)" : : "r" (&idt_reg));
    __asm__ volatile("sti");
}

void isr_handler(registers_t *regs) {
    if (regs->int_no == 32) {
        schedule();
    }
    else if (regs->int_no == 33) {
        keyboard_handler();
    }
    // FIX: Handle Mouse
    else if (regs->int_no == 44) {
        mouse_handler();
    }
    else if (regs->int_no == 128) {
        syscall_handler(regs);
    }
    
    // ACK PIC
    if (regs->int_no >= 32 && regs->int_no <= 47) {
        if (regs->int_no >= 40) outb(0xA0, 0x20); // Slave ACK
        outb(0x20, 0x20); // Master ACK
    }
}