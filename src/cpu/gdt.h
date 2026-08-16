/*
 * Global Descriptor Table API
 *
 * Exposes functions to initialize the GDT and update the Task State Segment (TSS).
 * Includes the full x86 hardware TSS structure definition.
 */
#ifndef GDT_H
#define GDT_H
#include <stdint.h>

void init_gdt();

// Task State Segment (TSS) Structure.
// The CPU uses this primarily during hardware task switching (which modern OSes avoid)
// or when switching privilege levels (e.g., handling an interrupt from ring 3 to ring 0).
// In a modern OS, we mostly only care about updating esp0 and ss0 when context switching.
typedef struct {
    uint32_t prev_tss;
    uint32_t esp0;       // Kernel stack pointer used during ring 3 -> ring 0 transition
    uint32_t ss0;        // Kernel stack segment
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base; // Offset to the I/O permission bit map
} __attribute__((packed)) tss_entry_t;

void                      tss_set_stack(uint32_t kss, uint32_t kesp);

#endif