/*
 * Interrupt Descriptor Table (IDT)
 *
 * Maps hardware interrupts and CPU exceptions to their respective handler functions.
 * When the CPU encounters an interrupt (like a timer tick, a key press, or a divide-by-zero error),
 * it uses this table to figure out what code to run next.
 */
#include "idt.h"
#include "../drivers/serial.h"
#include "../kernel/syscall.h"
#include "../gui/window.h"

// External assembly routines for each interrupt vector.
extern void       isr0();
extern void       isr1();
extern void       isr2();
extern void       isr3();
extern void       isr4();
extern void       isr5();
extern void       isr6();
extern void       isr7();
extern void       isr8();
extern void       isr9();
extern void       isr10();
extern void       isr11();
extern void       isr12();
extern void       isr13();
extern void       isr14();
extern void       isr15();
extern void       isr16();
extern void       isr17();
extern void       isr18();
extern void       isr19();
extern void       isr20();
extern void       isr128();
extern void       isr32();
extern void       isr33();
extern void       isr44();
extern void       isr46();
extern void       isr47();

// The table of 256 interrupt descriptors.
idt_entry_t       idt[256];
// The pointer structure needed to load the IDT into the CPU via 'lidt'.
idt_register_t    idt_reg;
volatile uint32_t system_ticks = 0;

extern void       keyboard_handler();
extern void       schedule();
extern void       mouse_handler();
extern void       term_print(const char *str);

// Helper to retrieve the value of CR2, which holds the address that caused a Page Fault.
uint32_t          get_cr2() {
    uint32_t val;
    __asm__ volatile("mov %%cr2, %0" : "=r"(val));
    return val;
}

// Converts an integer into a hex string and prints it to the terminal.
void term_print_hex(uint32_t n) {
    char *digits = "0123456789ABCDEF";
    char  str[11];
    str[0]  = '0';
    str[1]  = 'x';
    str[10] = '\0';
    for (int i = 0; i < 8; i++) {
        str[9 - i] = digits[n & 0xF];
        n >>= 4;
    }
    term_print(str);
}

// Configures a single entry in the IDT.
void set_idt_gate(int n, uint32_t handler) {
    idt[n].base_low  = handler & 0xFFFF;
    idt[n].sel       = 0x08; // Kernel code segment selector
    idt[n].always0   = 0;
    // 0x8E implies a 32-bit interrupt gate, present, and ring 0 (kernel) privilege.
    idt[n].flags     = 0x8E;
    idt[n].base_high = (handler >> 16) & 0xFFFF;
}

// Remaps the Programmable Interrupt Controller (PIC) to avoid conflicts with CPU exceptions.
// By default, the PIC maps hardware interrupts to vectors 0-15, which overlap with the
// CPU's internal exceptions (like divide-by-zero). This moves them to vectors 32-47.
void remap_pic() {
    outb(0x20, 0x11); // Start PIC1 initialization
    outb(0xA0, 0x11); // Start PIC2 initialization
    
    outb(0x21, 0x20); // Map PIC1 interrupts to vectors 32-39
    outb(0xA1, 0x28); // Map PIC2 interrupts to vectors 40-47
    
    outb(0x21, 0x04); // Configure PIC1 to cascade to PIC2
    outb(0xA1, 0x02); // Configure PIC2 cascade identity
    
    outb(0x21, 0x01); // 8086/88 mode for PIC1
    outb(0xA1, 0x01); // 8086/88 mode for PIC2
    
    outb(0x21, 0x00); // Unmask all interrupts on PIC1
    outb(0xA1, 0x00); // Unmask all interrupts on PIC2
}

// Sets up the IDT by configuring the exception and IRQ handlers.
void init_idt() {
    idt_reg.base  = (uint32_t)&idt;
    idt_reg.limit = 256 * sizeof(idt_entry_t) - 1;

    remap_pic();

    // Map the CPU exceptions
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

    // Map hardware IRQs
    set_idt_gate(32, (uint32_t)isr32); // Timer
    set_idt_gate(33, (uint32_t)isr33); // Keyboard
    set_idt_gate(44, (uint32_t)isr44); // Mouse

    // System call interrupt
    set_idt_gate(128, (uint32_t)isr128);
    // 0xEE sets the descriptor privilege level to 3, allowing user-space code to trigger it.
    idt[128].flags = 0xEE;

    set_idt_gate(46, (uint32_t)isr46); // ATA primary
    set_idt_gate(47, (uint32_t)isr47); // ATA secondary

    // Load the IDT and re-enable hardware interrupts
    __asm__ volatile("lidt (%0)" : : "r"(&idt_reg));
    __asm__ volatile("sti");
}

// Common C handler for interrupts and exceptions.
// The assembly stubs call this function, passing the state of the registers.
void isr_handler(registers_t *regs) {
    // Vectors below 32 are CPU exceptions (like page faults or division by zero).
    if (regs->int_no < 32) {
        term_print("\n[CPU EXCEPTION] Code: ");
        if (regs->int_no == 13) {
            term_print(" General Protection Fault!\n");
        } else if (regs->int_no == 14) {
            // Check if the page fault happened in user space (Code Segment 0x1B).
            if (regs->cs == 0x1B) {
                extern window_t *console_win;
                if (console_win)
                    console_win->text_color = 0xFFFF0000;
                term_print("\n========================================\n");
                term_print("[CORE DUMP] Segmentation Fault (Ring 3)\n");
                term_print("========================================\n");
                term_print("EIP (Instruction): ");
                term_print_hex(regs->eip);
                term_print("\n");
                term_print("EAX (Register)   : ");
                term_print_hex(regs->eax);
                term_print("\n");
                term_print("CR2 (Bad Address): ");
                term_print_hex(get_cr2());
                term_print("\n");
                term_print("========================================\n");
                term_print("Terminating process to protect Kernel...\n");
                if (console_win)
                    console_win->text_color = 0xFFFFFFFF;
                extern void process_exit(int code);
                // Kill the offending process to keep the system running.
                process_exit(139);
                return;
            }
            term_print(" Kernel Page Fault!\n");
            uint32_t faulting_addr = get_cr2();
            term_print(" CR2 (Faulting Address): ");
            term_print_hex(faulting_addr);
        }
        
        // A kernel-level exception means something went terribly wrong. Halt the system.
        term_print("\nSystem Halted.");
        for (;;)
            __asm__("hlt");
    }

    // Handle specific hardware IRQs and system calls.
    if (regs->int_no == 32) {
        system_ticks++;
        schedule(); // Trigger a context switch on timer ticks
    } else if (regs->int_no == 33) {
        keyboard_handler();
    } else if (regs->int_no == 44) {
        mouse_handler();
    } else if (regs->int_no == 128) {
        syscall_handler(regs);
    }
    
    if (regs->int_no == 46 || regs->int_no == 47) {
        // ATA interrupt handling goes here.
    }

    // Acknowledge the interrupt by sending an End of Interrupt (EOI) signal to the PIC.
    // This allows the PIC to send further interrupts.
    if (regs->int_no >= 32 && regs->int_no <= 47) {
        if (regs->int_no >= 40)
            outb(0xA0, 0x20); // Send EOI to secondary PIC
        outb(0x20, 0x20);     // Send EOI to primary PIC
    }
}