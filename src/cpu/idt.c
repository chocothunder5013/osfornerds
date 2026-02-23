/* src/cpu/idt.c */
#include "idt.h"
#include "../drivers/serial.h"
#include "../kernel/syscall.h"

extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr128(); // Syscall
extern void isr32();  // Timer
extern void isr33();  // Keyboard
// NEW: Extern declaration for Mouse Interrupt Stub (must be in isr_asm.S)
extern void isr44();
extern void isr46();
extern void isr47();

idt_entry_t idt[256];
idt_register_t idt_reg;

extern void keyboard_handler();
extern void schedule();
extern void mouse_handler();
extern void term_print(const char *str);

uint32_t get_cr2()
{
    uint32_t val;
    __asm__ volatile("mov %%cr2, %0" : "=r"(val));
    return val;
}

void term_print_hex(uint32_t n)
{
    char *digits = "0123456789ABCDEF";
    char str[11]; // "0x" + 8 digits + null
    str[0] = '0';
    str[1] = 'x';
    str[10] = '\0';

    for (int i = 0; i < 8; i++)
    {
        str[9 - i] = digits[n & 0xF];
        n >>= 4;
    }
    term_print(str);
}

void set_idt_gate(int n, uint32_t handler)
{
    idt[n].base_low = handler & 0xFFFF;
    idt[n].sel = 0x08;
    idt[n].always0 = 0;
    idt[n].flags = 0x8E;
    idt[n].base_high = (handler >> 16) & 0xFFFF;
}

void remap_pic()
{
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28); // Slave start 40 (0x28)
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x00);
    outb(0xA1, 0x00);
}

void init_idt()
{
    idt_reg.base = (uint32_t)&idt;
    idt_reg.limit = 256 * sizeof(idt_entry_t) - 1;

    remap_pic(); // Keep this!

    // Install Exception Handlers (0-31)
    set_idt_gate(0, (uint32_t)isr0);
    set_idt_gate(1, (uint32_t)isr1);
    set_idt_gate(2, (uint32_t)isr2);
    set_idt_gate(3, (uint32_t)isr3);
    set_idt_gate(4, (uint32_t)isr4);
    set_idt_gate(5, (uint32_t)isr5);
    set_idt_gate(6, (uint32_t)isr6);
    set_idt_gate(7, (uint32_t)isr7);
    set_idt_gate(8, (uint32_t)isr8);
    set_idt_gate(9, (uint32_t)isr9);
    set_idt_gate(10, (uint32_t)isr10);
    set_idt_gate(11, (uint32_t)isr11);
    set_idt_gate(12, (uint32_t)isr12);
    set_idt_gate(13, (uint32_t)isr13);
    set_idt_gate(14, (uint32_t)isr14);
    set_idt_gate(15, (uint32_t)isr15);
    set_idt_gate(16, (uint32_t)isr16);
    set_idt_gate(17, (uint32_t)isr17);
    set_idt_gate(18, (uint32_t)isr18);
    set_idt_gate(19, (uint32_t)isr19);
    set_idt_gate(20, (uint32_t)isr20);
    // Existing IRQs
    set_idt_gate(32, (uint32_t)isr32);
    set_idt_gate(33, (uint32_t)isr33);
    set_idt_gate(44, (uint32_t)isr44);
    set_idt_gate(128, (uint32_t)isr128);
    set_idt_gate(46, (uint32_t)isr46); // IRQ 14
    set_idt_gate(47, (uint32_t)isr47); // IRQ 15

    __asm__ volatile("lidt (%0)" : : "r"(&idt_reg));
    __asm__ volatile("sti");
}

void isr_handler(registers_t *regs)
{
    // 1. Handle CPU Exceptions (0-31)
    if (regs->int_no < 32)
    {
        term_print("\n[CPU EXCEPTION] Code: ");
        // You might need a number-to-string function here
        // e.g., term_print_dec(regs->int_no);

        if (regs->int_no == 13)
        {
            term_print(" General Protection Fault!\n");
        }
        else if (regs->int_no == 14)
        {
            term_print(" Page Fault!\n");
            uint32_t faulting_addr = get_cr2();
            term_print(" CR2 (Faulting Address): ");
            term_print_hex(faulting_addr);

            term_print("\n Error Code: ");
            // term_print_hex(regs->err_code);
            /* Error Code bits:
               Bit 0 (P): 0 = Non-present page, 1 = Protection violation
               Bit 1 (W): 0 = Read, 1 = Write
               Bit 2 (U): 0 = Kernel, 1 = User
            */
        }

        // HALT THE SYSTEM so we can read the error
        term_print("\nSystem Halted.");
        for (;;)
            __asm__("hlt");
    }

    // 2. Handle IRQs (32+)
    if (regs->int_no == 32)
    {
        schedule();
    }
    else if (regs->int_no == 33)
    {
        keyboard_handler();
    }
    else if (regs->int_no == 44)
    {
        mouse_handler();
    }
    else if (regs->int_no == 128)
    {
        syscall_handler(regs);
    }

    if (regs->int_no == 46 || regs->int_no == 47)
    {
        // We can leave this empty for now, or print "Disk IRQ"
        // The important part is the ACK at the bottom of this function.
    }

    // ACK PIC
    if (regs->int_no >= 32 && regs->int_no <= 47)
    {
        if (regs->int_no >= 40)
            outb(0xA0, 0x20);
        outb(0x20, 0x20);
    }
}