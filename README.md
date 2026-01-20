# OSForNerds - A Complete x86 Operating System

A comprehensive, from-scratch x86 operating system kernel written in C and Assembly, featuring advanced components like process management, virtual memory, ELF loader, file system support, graphics, and a window manager. This project demonstrates deep knowledge of OS architecture, low-level programming, and system design.

## Project Overview

**OSForNerds** is a fully functional 32-bit x86 operating system kernel that boots via Multiboot (GRUB/Limine) and provides:

- **Bootloader Support**: Multiboot-compliant boot with Limine BIOS/UEFI boot manager
- **CPU Management**: GDT (Global Descriptor Table), IDT (Interrupt Descriptor Table), ISR handling
- **Memory Management**: Physical Memory Manager (PMM), Virtual Memory Manager (VMM), kernel heap
- **Process Management**: Process creation, execution, context switching
- **File System**: FAT/custom filesystem support with file operations
- **Device Drivers**: Serial port, keyboard, mouse, graphics, ATA disk controller
- **User Programs**: Support for loading and executing user-space ELF binaries
- **GUI Components**: Window manager with basic graphics support
- **Shell**: Interactive command shell for system control

## Directory Structure

```
osfornerds/
├── README.md                 # Project documentation
├── Makefile                  # Build configuration and targets
├── linker.ld                 # Kernel linker script (memory layout)
├── limine.cfg                # Limine bootloader configuration
├── limine.conf               # Alternative Limine config
├── linker.ld                 # Linker script for kernel
├── scraper.py                # Utility script for development
│
├── src/                      # Core kernel source code
│   ├── kernel/               # Main kernel logic
│   │   ├── main.c            # Kernel entry point (kmain)
│   │   ├── boot.S            # Assembly bootloader code (_start)
│   │   ├── shell.c           # Command shell implementation
│   │   ├── process.c         # Process management (fork, exec)
│   │   ├── process.h         # Process structures and APIs
│   │   ├── elf.c             # ELF binary loader
│   │   ├── elf.h             # ELF structures
│   │   ├── fs.c              # File system implementation
│   │   ├── fs.h              # File system structures
│   │   ├── syscall.c         # System call handlers
│   │   ├── syscall.h         # System call definitions
│   │   ├── utils.c           # Utility functions (string ops, etc.)
│   │   ├── multiboot.h       # Multiboot protocol definitions
│   │
│   ├── cpu/                  # CPU and interrupt handling
│   │   ├── gdt.c             # GDT setup and management
│   │   ├── gdt.h             # GDT structures
│   │   ├── gdt_flush.S       # Assembly: load GDT
│   │   ├── idt.c             # IDT setup and interrupt handlers
│   │   ├── idt.h             # IDT structures
│   │   ├── isr_asm.S         # Assembly: interrupt service routines
│   │
│   ├── mm/                   # Memory management
│   │   ├── pmm.c             # Physical memory manager
│   │   ├── heap.c            # Dynamic memory allocation
│   │   ├── heap.h            # Heap structures
│   │   ├── vmm.c             # Virtual memory manager (paging)
│   │   ├── vmm.h             # VMM structures
│   │
│   ├── drivers/              # Hardware device drivers
│   │   ├── serial.c          # Serial port I/O (for debugging)
│   │   ├── serial.h          # Serial API
│   │   ├── keyboard.c        # Keyboard input handler
│   │   ├── mouse.c           # Mouse input handler
│   │   ├── vga.c             # VGA text mode driver
│   │   ├── vga.h             # VGA structures
│   │   ├── graphics.c        # Graphics/framebuffer support
│   │   ├── font.c            # Font rendering
│   │   ├── font.h            # Font structures
│   │   ├── ata.c             # ATA disk controller driver
│   │   ├── ata.h             # ATA structures
│   │   ├── rtc.c             # Real-time clock
│   │
│   └── gui/                  # Graphics and window management
│       ├── wm.c              # Window manager implementation
│       └── window.h          # Window structures
│
├── programs/                 # User-space programs
│   ├── entry.S               # User program entry point
│   ├── stdlib.c              # User-space standard library
│   ├── stdlib.h              # User-space stdlib header
│   ├── hello.c               # Hello World program
│   ├── echo.c                # Echo command
│   ├── cat.c                 # File read/display command
│   ├── ls.c                  # Directory listing
│   ├── date.c                # Date/time command
│   ├── kedit.c               # Kernel text editor
│   ├── memtest.c             # Memory test utility
│   └── linker.ld             # User program linker script
│
├── limine/                   # Limine bootloader binary/resources
│   ├── limine               # Limine bootloader executable
│   ├── BOOTX64.EFI          # UEFI x86_64 boot file
│   ├── BOOTIA32.EFI         # UEFI IA32 boot file
│   ├── BOOTAA64.EFI         # UEFI ARM64 boot file
│   ├── BOOTRISCV64.EFI      # UEFI RISC-V boot file
│   ├── limine-bios.sys      # BIOS boot file
│   ├── limine-bios-hdd.h    # BIOS HDD header
│   ├── limine.c             # Limine protocol implementation
│   ├── limine.h             # Limine protocol header
│   ├── Makefile             # Limine build config
│   └── LICENSE              # Limine license
│
└── iso_root/                # ISO image root (for bootable disc)
    ├── limine-bios.sys      # BIOS bootloader
    ├── limine.conf          # Bootloader configuration
    └── test.txt             # Test data file
```

## Build System

### Compilation Process

The kernel is compiled using a cross-compiler targeting i686 ELF:

```bash
CC = i686-elf-gcc
LD = i686-elf-ld
NASM = nasm
```

**Kernel Compilation Flags:**
- `-m32`: 32-bit compilation
- `-ffreestanding`: No standard library dependency
- `-O2`: Optimization level
- `-Wall -Wextra`: Warnings
- `-mgeneral-regs-only`: Restrict to general registers

**Kernel Assembly Sources:**
1. `src/kernel/boot.S` - Bootloader entry point (_start)
2. `src/cpu/gdt_flush.S` - GDT loader
3. `src/cpu/isr_asm.S` - Interrupt handlers

**Kernel C Sources:** ~20 files including process, memory, filesystem, and driver code

**Output:** `kernel.bin` - Raw kernel binary (loaded at 0x200000)

### Key Build Targets

- `make` or `make kernel` - Build kernel
- `make clean` - Remove build artifacts
- `make iso` - Create bootable ISO image
- `make run` - Run in emulator
- `make programs` - Build user programs

## Architecture Details

### Memory Layout

```
0x00000000 - 0x00000FFF : Real mode IVT (legacy)
0x00001000 - 0x000FFFFF : Low memory (device memory, etc.)
0x00100000 - 0x001FFFFF : Boot region
0x00200000 - 0xFFFFFFFF : Kernel space (paging enabled)
```

**Linker Script** (`linker.ld`):
- Kernel loaded at **0x200000** (2MB)
- Sections: `.multiboot`, `.text`, `.data`, `.bss`

### CPU Features

**GDT (Global Descriptor Table):**
- Null descriptor
- Kernel code segment (Ring 0, 32-bit)
- Kernel data segment (Ring 0, 32-bit)
- User code segment (Ring 3)
- User data segment (Ring 3)
- TSS (Task State Segment)

**IDT (Interrupt Descriptor Table):**
- 256 interrupt/exception handlers
- Exceptions (0-31): Faults, traps, aborts
- Hardware IRQs (32-47): PIC-controlled (timer, keyboard, etc.)
- System calls (0x80): User-initiated syscalls

### Memory Management

**Physical Memory Manager (PMM):**
- Tracks available physical memory via bitmap
- Allocates/deallocates page frames (4KB)
- Initialized with Multiboot memory map

**Virtual Memory Manager (VMM):**
- Page tables and page directories
- Virtual address to physical address translation
- Paging enabled for kernel and user space

**Heap:**
- Dynamic memory allocation (`malloc`/`free`)
- Kernel heap management with free block lists
- Growing heap with demand paging

### Processes & Syscalls

**Process Management:**
- Process Control Block (PCB) structure
- Process states: ready, running, blocked
- Context switching via timer interrupt
- Fork/exec support for spawning processes

**System Calls (via INT 0x80):**
- `SYS_PRINT` - Print to console
- `SYS_READ` - Read from device
- `SYS_WRITE` - Write to device
- File I/O operations
- Memory management calls

### File System

- File operations: open, read, write, close, seek
- Directory operations: list, create, delete
- File metadata tracking
- Integration with disk drivers (ATA)

### Device Drivers

**Serial Driver:**
- COM1 port communication
- 115200 baud, 8N1
- Used for kernel logging/debugging

**Keyboard Driver:**
- Scancodes to ASCII conversion
- Interrupt-driven input handling
- Key press/release detection

**VGA/Graphics Driver:**
- VGA text mode (80x25)
- Graphics framebuffer support
- Font rendering

**ATA Driver:**
- IDE/SATA disk access
- DMA and PIO modes
- Partition detection

**Mouse Driver:**
- PS/2 mouse support
- Position tracking
- Button state management

### Window Manager & GUI

**Window Manager (WM):**
- Window structure and lifecycle
- Window positioning and sizing
- Event routing (keyboard, mouse)
- Rendering pipeline

**Window Structures:**
- Content buffer
- Event callbacks
- Parent/child relationships

## Booting Process

1. **BIOS/UEFI** - Firmware loads Limine bootloader
2. **Limine** - Bootloader loads kernel to 0x200000, provides Multiboot structure
3. **Boot.S** (`_start`) - Asm code sets up:
   - Stack pointer
   - Calls kmain (kernel entry in main.c)
4. **kmain** - Initializes:
   - Serial port (debugging)
   - GDT and IDT setup
   - Physical/virtual memory
   - Heap
   - File system
   - Device drivers
   - Starts shell/processes

## User Programs

Located in `programs/`, compiled to ELF binaries:

- **hello.c** - "Hello, World!" example
- **echo.c** - Echo text command
- **cat.c** - File reading/display
- **ls.c** - Directory listing
- **date.c** - Display system time
- **kedit.c** - Text editor
- **memtest.c** - Memory diagnostics
- **stdlib.c** - User-space C library (malloc, strcpy, etc.)

Programs link via `programs/linker.ld` and are loaded/executed via the ELF loader.

## Building & Running

### Prerequisites

1. **Cross-compiler**: i686-elf-gcc, i686-elf-ld
   ```bash
   # Download/install or use system package manager
   ```

2. **Tools**: NASM, Limine bootloader
3. **Emulator**: QEMU or similar

### Build Steps

```bash
cd osfornerds

# Build kernel and programs
make

# Create ISO image
make iso

# Run in QEMU
make run

# Clean build artifacts
make clean
```

### Output Files

- `kernel.bin` - Kernel binary
- `kernel.iso` - Bootable ISO image
- `programs/*.bin` - User program binaries

## Development Notes

### Adding New Features

1. **New syscall**: Add to `src/kernel/syscall.c` and define in `syscall.h`
2. **New driver**: Create in `src/drivers/`, add to Makefile
3. **New user program**: Add `.c` file to `programs/`, update programs Makefile section

### Debugging

- Kernel logs via serial: Check QEMU serial output
- GDB support available via `-s -S` QEMU flags
- Breakpoints in Assembly/C code

### Code Style

- C89/C99 compatible
- Snake_case for functions/variables
- UPPERCASE for macros/constants
- Comments for complex logic

## Technical Highlights

- **Pure C/ASM implementation** - No external OS abstraction
- **Full memory protection** - Paging and segmentation
- **Interrupt-driven I/O** - Keyboard, mouse, timer
- **Process isolation** - User/kernel space separation
- **Dynamic loading** - ELF binary support
- **Modular architecture** - Cleanly separated drivers/subsystems

## Limitations & Future Work

- **No SMP** - Single CPU support only
- **Basic filesystem** - Simple FAT-like structure
- **Limited syscalls** - Subset of POSIX
- **No networking** - No network drivers
- **16MB max RAM** - Addressable physical memory (expandable)

### Planned Enhancements

- Multi-core CPU support
- Advanced filesystem (ext2)
- Network stack (TCP/IP)
- More sophisticated process scheduling
- POSIX compliance improvement

## References

- Intel IA-32 Architecture Manual
- Multiboot Specification
- OSDev.org tutorials and resources
- Linux kernel architecture principles

## License

This project is educational and open-source. Refer to individual components for specific licenses (Limine has its own LICENSE).

---

**Created**: OS Enthusiasts & System Programming Community  
**Last Updated**: January 2026  
**Status**: Active Development
