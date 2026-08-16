# Compiler and Assembler Configuration
CC = gcc
NASM = nasm

# Kernel Build Flags
# -m32: Compile for 32-bit architecture
# -ffreestanding: Target an environment without a standard library
# -mgeneral-regs-only: Avoid using floating-point or vector registers
# -fno-pic -fno-pie: Disable position-independent code (kernel is loaded at a fixed address)
CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -Isrc -mgeneral-regs-only -fno-pic -fno-pie

# Kernel Linker Flags
# -T linker.ld: Use custom linker script for memory layout
# -nostdlib: Do not link standard C library
# -Wl,--build-id=none: Remove build ID notes to save space
LDFLAGS = -m32 -T linker.ld -ffreestanding -O2 -nostdlib -no-pie -Wl,--build-id=none -lgcc

# Assembler Flags
# -f elf32: Output 32-bit ELF object files
NASMFLAGS = -f elf32

# Userland Build Flags
# Similar to kernel flags, but include paths point to the 'programs' directory
USER_CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -Iprograms -mgeneral-regs-only -fno-pic -fno-pie
USER_LDFLAGS = -m32 -T programs/linker.ld -ffreestanding -O2 -nostdlib -no-pie -lgcc

# Source File Definitions
C_SOURCES = src/kernel/main.c         src/kernel/utils.c         src/drivers/serial.c         src/drivers/keyboard.c         src/drivers/vga.c         src/cpu/gdt.c         src/kernel/shell.c         src/cpu/idt.c         src/kernel/elf.c         src/kernel/fs.c         src/mm/pmm.c         src/mm/vmm.c         src/mm/heap.c         src/drivers/graphics.c         src/drivers/font.c         src/drivers/mouse.c         src/drivers/ata.c         src/kernel/syscall.c         src/kernel/process.c         src/gui/wm.c
ASM_SOURCES = src/kernel/boot.S           src/cpu/gdt_flush.S           src/cpu/isr_asm.S
OBJ = $(C_SOURCES:.c=.o) $(ASM_SOURCES:.S=.o)

# Userland Applications to Build
USER_APPS = cat date echo hello kedit ls memtest sysmon crash stress
USER_ELFS = $(USER_APPS:=.elf)
USER_LIBS = programs/entry.o programs/stdlib.o

# Default Target
all: my-os.iso

# Link Kernel Executable
my-kernel.elf: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

# Compile Kernel C Sources
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble Kernel Assembly Sources
%.o: %.S
	$(NASM) $(NASMFLAGS) $< -o $@

# Compile Userland C Sources
programs/%.o: programs/%.c
	$(CC) $(USER_CFLAGS) -c $< -o $@

# Assemble Userland Entry Point
programs/entry.o: programs/entry.S
	$(NASM) -f elf32 $< -o $@

# Link Userland Executables
$(USER_ELFS): %.elf: programs/%.o $(USER_LIBS) programs/linker.ld
	$(CC) $(USER_LDFLAGS) -o $@ $(USER_LIBS) programs/$*.o

# Download and Build Limine Bootloader
limine:
	git clone https://github.com/limine-bootloader/limine.git --branch=v8.x-binary --depth=1
	make -C limine

# Build File System Image
disk.img: $(USER_ELFS)
	echo "Hello from the filesystem! This text is loaded from disk." > test.txt
	python3 build_fs.py disk.img $(USER_ELFS) test.txt

# Create Bootable ISO Image
my-os.iso: my-kernel.elf limine $(USER_ELFS) disk.img
	rm -rf iso_root
	mkdir -p iso_root
	cp my-kernel.elf iso_root/
	echo "Hello from the filesystem! This text is loaded from disk." > iso_root/test.txt
	cp limine.conf iso_root/
	cp $(USER_ELFS) iso_root/
	cp limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-uefi-cd.bin iso_root/
	xorriso -as mkisofs -b limine-bios-cd.bin 		-no-emul-boot -boot-load-size 4 -boot-info-table 		--efi-boot limine-uefi-cd.bin 		-efi-boot-part --efi-boot-image --protective-msdos-label 		iso_root -o my-os.iso
	./limine/limine bios-install my-os.iso

# Run in QEMU Emulator
run: my-os.iso
	qemu-system-i386 -vga std -cdrom my-os.iso -drive file=disk.img,format=raw,index=0,media=disk -serial file:serial.log -d int,cpu_reset -D qemu.log

# Clean Build Artifacts
clean:
	rm -rf src/**/*.o src/kernel/*.o src/cpu/*.o src/drivers/*.o src/mm/*.o
	rm -rf programs/*.o
	rm -rf *.elf *.iso iso_root limine disk.img qemu.log serial.log
