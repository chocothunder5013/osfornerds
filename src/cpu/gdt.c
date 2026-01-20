/* src/cpu/gdt.c */
#include "gdt.h"

// Define the GDT structures locally
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct gdt_entry gdt[6]; // Null, KCode, KData, UCode, UData, TSS
struct gdt_ptr gp;
tss_entry_t tss_entry;

extern void gdt_flush(uint32_t);
extern void tss_flush(); 

void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;
    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F);
    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access      = access;
}

void write_tss(int num, uint16_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t) &tss_entry;
    uint32_t limit = sizeof(tss_entry);

    // TSS Descriptor
    // Access: Present (1), Ring 0 (00), Executable (0), Type (9 = Available TSS 32-bit) -> 0xE9?
    // Actually standard TSS Byte: 10001001 = 0x89
    // 0xE9 is for Ring 3? No, TSS is a system segment.
    // Let's use 0xE9 for "Present, Ring 3, Accessed" if we want user to trigger it (rare)
    // Standard kernel TSS: 0x89 (Present, Ring 0, Type 9)
    gdt_set_gate(num, base, limit, 0x89, 0x00); 

    // Zero out
    uint8_t *p = (uint8_t *)&tss_entry;
    for(int i=0; i<sizeof(tss_entry); i++) p[i] = 0;

    tss_entry.ss0  = ss0; 
    tss_entry.esp0 = esp0; 
    
    // I/O Map Base > Limit disables I/O permission bitmap
    tss_entry.iomap_base = sizeof(tss_entry);
}

void init_gdt() {
    gp.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gp.base  = (uint32_t)&gdt;

    // 0: Null
    gdt_set_gate(0, 0, 0, 0, 0);

    // 1: Kernel Code (0x08)
    // Base=0, Limit=4GB, Access=0x9A (Present, Ring0, Code, Read), Gran=0xCF (4KB blocks)
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    // 2: Kernel Data (0x10)
    // Access=0x92 (Present, Ring0, Data, Write)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    // 3: User Code (0x18)
    // Access=0xFA (Present, Ring3, Code, Read)
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);

    // 4: User Data (0x20)
    // Access=0xF2 (Present, Ring3, Data, Write)
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    // 5: TSS (0x28)
    // Initial Kernel Stack is arbitrary, will be updated by scheduler
    write_tss(5, 0x10, 0x0);

    gdt_flush((uint32_t)&gp);
    tss_flush(); // Load TR register
}

// Critical: Called by scheduler to update Kernel Stack for the NEXT interrupt
void tss_set_stack(uint32_t kss, uint32_t kesp) {
    tss_entry.ss0 = kss;
    tss_entry.esp0 = kesp;
}