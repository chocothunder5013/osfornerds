/*
 * Interrupt Descriptor Table API
 *
 * Defines the structures used by the IDT and the register state
 * passed to interrupt handlers.
 */
#ifndef IDT_H
#define IDT_H
#include <stdint.h>

// Represents the state of the CPU registers pushed onto the stack
// by the assembly interrupt stubs before calling the C handler.
typedef struct {
    uint32_t ds; // Data segment selector
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by 'pusha'
    uint32_t int_no, err_code; // Interrupt number and CPU error code
    uint32_t eip, cs, eflags, useresp, ss; // Pushed by the processor automatically
} registers_t;

void init_idt();

// Defines the pointer structure required by the `lidt` instruction to load the IDT.
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_register_t;

// Defines a single entry in the IDT.
typedef struct {
    uint16_t base_low;  // Lower 16 bits of the handler function address
    uint16_t sel;       // Kernel segment selector
    uint8_t  always0;   // This must always be zero
    uint8_t  flags;     // Type and attribute flags
    uint16_t base_high; // Upper 16 bits of the handler function address
} __attribute__((packed)) idt_entry_t;

#endif