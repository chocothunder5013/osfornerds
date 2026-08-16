# OSForNerds

![C](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white)
![Assembly](https://img.shields.io/badge/Assembly-100000?style=for-the-badge)
![Limine](https://img.shields.io/badge/Limine-Bootloader-4CAF50?style=for-the-badge)
![QEMU](https://img.shields.io/badge/QEMU-Tested-FF6600?style=for-the-badge)

OSForNerds is a 32-bit x86 monolithic kernel and operating system built from scratch. It has a preemptive multitasking scheduler, virtual memory isolation, a custom flat virtual file system (VFS), and a hardware-accelerated compositing window manager for executing Ring 3 ELF binaries.

![OS Screenshot Placeholder](https://via.placeholder.com/800x400.png?text=Replace+this+with+a+screenshot+of+your+OS+running+in+QEMU)

---

## Technical Highlights

* **Preemptive Multitasking:** Round-robin scheduling driven by hardware interrupts, managing Ring 0 threads and Ring 3 userland processes.
* **Memory Isolation:** Two-tier paging system (VMM) providing per-process isolated virtual address spaces and a robust kernel heap (`kmalloc`/`kfree`) with dynamic coalescing.
* **Custom GUI Compositor:** A double-buffered, hardware-accelerated window manager that eliminates screen tearing, featuring floating overlapping windows and a dynamic 8x8 font renderer.
* **Flat Virtual Filesystem:** A RAM-cached, hierarchal VFS backed by an ATA PIO disk driver for persistent sector-level storage.
* **Ring 3 Userland:** Executes standard 32-bit ELF binaries linked against a custom standard library, communicating with the kernel via a definitive `INT 0x80` syscall ABI.

---

## Getting Started

### 1. Install Dependencies
Ensure you have the required 32-bit compilation tools, NASM, and QEMU installed.

**Debian / Ubuntu / WSL:**
```bash
sudo apt update
sudo apt install build-essential nasm xorriso qemu-system-x86 qemu-utils git mtools gcc-multilib

```

**Fedora / RHEL:**

```bash
sudo dnf update
sudo dnf install make gcc nasm xorriso qemu-system-x86 qemu-img git mtools glibc-devel.i686 libgcc.i686

```

### 2. Build & Run

The `Makefile` handles fetching the Limine bootloader, compiling the kernel and userland, generating the filesystem disk, and creating the bootable ISO.

```bash
# Compile everything and build the ISO
make all

# Launch the OS in QEMU
make run

```

> **Note:** System logs and CPU state dumps (including IDT segfault captures) are piped directly to `serial.log` and `qemu.log` in the root directory.

---

## Architecture & Documentation

Read more about the OS architecture:

* **[System Architecture & Boot Sequence](./docs/architecture.md)**
* **[Window Manager & Compositor](./docs/window_manager.md)**
* **[Filesystem & ATA Storage](./docs/filesystem.md)**
* **[System Call (Syscall) API](./docs/syscalls.md)**

---

## Userland Environment

OSForNerds boots into a graphical terminal hosting an interactive shell. It supports standard built-in commands and executes ELF binaries natively from disk.

| Command | Description |
| --- | --- |
| `ls [path]` | Lists directory contents. |
| `cd <path>` | Changes current working directory. |
| `cat <file>` | Prints text file contents to the terminal. |
| `mkdir <name>` | Creates a new directory on the VFS. |
| `rm <file>` | Deletes a file. |
| `<program.elf>` | Executes a userland binary (e.g., `sysmon.elf`, `kedit.elf`). |

### Diagnostic Suites

The userland includes built-in stress-testing tools:

* `sysmon.elf`: A graphical resource monitor tracking uptime, task counts, and live memory usage.
* `stress.elf`: A chaos suite that runs CPU burners and memory leaks to test the kernel's **OOM Killer**.
* `crash.elf`: Purposefully triggers a Ring 3 Segmentation Fault to test IDT protection.
