```markdown
# MyOS

A custom, bare-metal 32-bit x86 operating system written in C and Assembly. This project features a graphical window manager, cooperative multitasking, custom PS/2 device drivers, and a standalone userland environment bootstrapped via the Multiboot1 protocol.

## Features

* **Bootloader:** Bootstrapped using Limine (v8.x) via the Multiboot1 protocol.
* **Memory Management:** Implements Physical Memory Management (PMM), Virtual Memory Management (VMM) with paging, and a custom linked-list kernel heap (`kmalloc`/`kfree`).
* **Multitasking:** Custom scheduler managing Ring 0 kernel threads and process states (Ready, Blocked, Zombie) with cooperative yielding.
* **Graphical User Interface:** Utilizes VBE linear framebuffers. Features a double-buffered compositor, a basic window manager, and 8x8 VGA bitmap font rendering.
* **Hardware Drivers:** Custom PS/2 Keyboard and Mouse drivers with hardware interrupt handling (PIC/IDT), scancode translation, and packet synchronization.
* **Userland:** Includes a custom standard library (`stdlib.c`) handling system calls (via `int 0x80`), and a suite of modular programs (Shell, Text Editor, File Utilities) loaded into memory as Multiboot modules.

## Prerequisites

To build and run the operating system, you will need a Linux environment (or WSL) with the following dependencies installed. The build system uses the host GCC compiler with multilib support for 32-bit cross-compilation.

**Debian / Ubuntu:**
```bash
sudo apt update
sudo apt install build-essential nasm xorriso qemu-system-x86 qemu-utils git mtools gcc-multilib
```

## Build and Installation

The project uses GNU Make to orchestrate the compilation of the kernel, the assembly of user programs, the fetching of the bootloader, and the generation of the bootable ISO.

1.  **Clone the repository:**
    ```bash
    git clone <your-repository-url>
    cd <repository-directory>
    ```

2.  **Build the OS and generate the ISO:**
    ```bash
    make all
    ```
    *This command compiles the kernel, builds the user programs into ELF binaries, clones the Limine bootloader, and packages everything into `my-os.iso`.*

3.  **Run in QEMU:**
    ```bash
    make run
    ```

4.  **Clean the build environment:**
    ```bash
    make clean
    ```

## Operation

Upon execution, QEMU will launch and the Limine bootloader will load the kernel and userland modules into memory. The kernel will initialize the Global Descriptor Table (GDT), Interrupt Descriptor Table (IDT), memory allocators, and hardware drivers before handing control to the multitasking scheduler.

The OS defaults to a graphical desktop environment displaying a central terminal window. 

### Shell Commands
The default shell task supports the following commands:
* `help` - Display available commands.
* `ls [path]` - List directory contents.
* `cd <path>` - Change current working directory.
* `pwd` - Print working directory.
* `cat <file>` - Print file contents to the terminal.
* `touch <file>` - Create a new file.
* `mkdir <name>` - Create a new directory.
* `rm <file>` - Delete a file.
* `clear` - Clear the terminal screen.
* `<program_name>` - Execute a userland ELF binary (e.g., `hello.elf`, `kedit.elf`).

### System Logs
The Makefile is configured to output kernel debugging information over the serial port. You can monitor the system state in real-time by inspecting the generated `serial.log` and `qemu.log` files in the root directory.

## Project Structure

* `src/kernel/` - Core initialization, multitasking scheduler, syscall interface, and virtual filesystem.
* `src/cpu/` - x86 architecture specifics (GDT, IDT, ISRs).
* `src/mm/` - Memory management (PMM, VMM, Heap).
* `src/drivers/` - Hardware interaction (ATA, PS/2 Keyboard/Mouse, VGA, Serial).
* `src/gui/` - Window manager, compositor, and font rendering.
* `programs/` - Userland C applications, assembly entry points, and the custom standard library.
* `limine.conf` - Bootloader configuration specifying the kernel and Multiboot modules.
* `linker.ld` - Kernel linker script ensuring proper memory alignment and Multiboot header placement.
```