/* src/cpu/idt.h */

#ifndef IDT_H
#define IDT_H

#include <stdint.h>


// Add this struct before the function declarations
typedef struct {
    uint32_t ds;                                     // Data segment selector
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha
    uint32_t int_no, err_code;                       // Interrupt number and error code
    uint32_t eip, cs, eflags, useresp, ss;           // Pushed by the processor automatically
} registers_t;
// Initialize IDT and PIC
void init_idt();

// Struct for the IDT Register (similar to GDT ptr)
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_register_t;

// Struct for an IDT Entry
typedef struct {
    uint16_t base_low;
    uint16_t sel;        // Kernel Segment Selector (0x08)
    uint8_t  always0;    // Always 0
    uint8_t  flags;      // Flags (Present, Ring 0, etc)
    uint16_t base_high;
} __attribute__((packed)) idt_entry_t;

#endif