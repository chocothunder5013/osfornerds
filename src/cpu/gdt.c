/*
 * Global Descriptor Table (GDT)
 *
 * Configures memory segmentation required by the x86 architecture
 * and sets up the Task State Segment (TSS) for context switching.
 * 
 * The GDT defines the characteristics of various memory segments used during execution,
 * such as their base addresses, sizes, and access privileges (e.g., ring 0 vs ring 3).
 * Even though paging is typically used for memory management in modern OSes, x86 hardware
 * still requires a basic GDT setup to transition between privilege levels.
 */
#include "gdt.h"

// Defines a single entry in the Global Descriptor Table.
// Due to historical x86 quirks, the base and limit fields are fragmented across the structure.
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;      // Access flags (e.g., executable, read/write, privilege level)
    uint8_t  granularity; // Granularity flags (e.g., 1B vs 4KB scaling) and top 4 bits of the limit
    uint8_t  base_high;
} __attribute__((packed));

// Defines the pointer structure required by the `lgdt` instruction to load the GDT.
struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// Array of 6 GDT entries:
// 0: Null descriptor (required)
// 1: Kernel Code Segment
// 2: Kernel Data Segment
// 3: User Code Segment
// 4: User Data Segment
// 5: Task State Segment (TSS)
struct gdt_entry gdt[6];
struct gdt_ptr   gp;
tss_entry_t      tss_entry;

extern void      gdt_flush(uint32_t);
extern void      tss_flush();

// Helper function to populate a specific GDT entry.
// It reconstructs the fragmented base and limit values into the hardware-expected format.
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;

    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F);

    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access = access;
}

// Configures the Task State Segment (TSS).
// The TSS provides the CPU with the stack pointer (ESP0) and stack segment (SS0)
// to use when a hardware interrupt or system call transitions execution from user mode (Ring 3)
// to kernel mode (Ring 0). Without this, the CPU wouldn't know where to save the user state.
void write_tss(int num, uint16_t ss0, uint32_t esp0) {
    uint32_t base  = (uint32_t)&tss_entry;
    uint32_t limit = sizeof(tss_entry);

    // Create the TSS descriptor in the GDT.
    // Access 0x89 indicates a 32-bit available TSS.
    gdt_set_gate(num, base, limit, 0x89, 0x00);

    // Ensure the TSS is clean.
    uint8_t *p = (uint8_t *)&tss_entry;
    for (int i = 0; i < sizeof(tss_entry); i++)
        p[i] = 0;

    // Set the kernel stack segment and pointer for interrupt handling.
    tss_entry.ss0        = ss0;
    tss_entry.esp0       = esp0;
    
    // Set the I/O map base address beyond the TSS size to deny user-mode I/O port access.
    tss_entry.iomap_base = sizeof(tss_entry);
}

// Initializes standard kernel and user mode segments.
// This sets up a flat memory model where all segments start at 0 and span the full 4GB memory space.
void init_gdt() {
    gp.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gp.base  = (uint32_t)&gdt;

    // The required null descriptor.
    gdt_set_gate(0, 0, 0, 0, 0);

    // Segment 1: Kernel Code (Access 0x9A = Present, Ring 0, Executable, Readable)
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    // Segment 2: Kernel Data (Access 0x92 = Present, Ring 0, Data, Writable)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    // Segment 3: User Code (Access 0xFA = Present, Ring 3, Executable, Readable)
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);

    // Segment 4: User Data (Access 0xF2 = Present, Ring 3, Data, Writable)
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    // Segment 5: TSS. We initialize the kernel stack to 0x0 for now; it gets updated during task switches.
    write_tss(5, 0x10, 0x0);

    // Tell the CPU about the new GDT.
    gdt_flush((uint32_t)&gp);
    
    // Tell the CPU to load the TSS.
    tss_flush();
}

// Updates the kernel stack pointer within the TSS.
// This must be called during a context switch so the CPU knows where to push state
// when the next interrupt occurs while the new process is executing in user mode.
void tss_set_stack(uint32_t kss, uint32_t kesp) {
    tss_entry.ss0  = kss;
    tss_entry.esp0 = kesp;
}