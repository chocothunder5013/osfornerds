# Makefile - Final "Deep Tech" Version
# Ensure $HOME/opt/cross/bin is in your PATH

CC = i686-elf-gcc
LD = i686-elf-ld
NASM = nasm 

# --- Compiler Flags ---
# Kernel Flags
CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -Isrc -mgeneral-regs-only
LDFLAGS = -m elf_i386 -T linker.ld
NASMFLAGS = -f elf32

# User Program Flags
USER_CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -Iprograms -mgeneral-regs-only
USER_LDFLAGS = -m elf_i386 -T programs/linker.ld

# --- Source Files ---
# Kernel Sources (boot.S must be first in ASM list usually, but linker handles order)
C_SOURCES = src/kernel/main.c \
	    src/kernel/utils.c \
	    src/drivers/serial.c \
	    src/drivers/keyboard.c \
	    src/drivers/vga.c \
	    src/cpu/gdt.c \
	    src/kernel/shell.c \
	    src/cpu/idt.c \
	    src/kernel/elf.c \
	    src/kernel/fs.c \
	    src/mm/pmm.c \
	    src/mm/vmm.c \
	    src/mm/heap.c \
	    src/drivers/graphics.c \
	    src/drivers/font.c \
	    src/drivers/mouse.c \
	    src/drivers/ata.c \
	    src/kernel/syscall.c \
	    src/kernel/process.c \
	    src/gui/wm.c

ASM_SOURCES = src/kernel/boot.S \
	      src/cpu/gdt_flush.S \
	      src/cpu/isr_asm.S

OBJ = $(C_SOURCES:.c=.o) $(ASM_SOURCES:.S=.o)

# User Sources
USER_OBJS = programs/entry.o programs/stdlib.o programs/hello.o

# --- Main Targets ---

# Default target: Build everything
all: my-os.iso

# Link the kernel
my-kernel.elf: $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

# Compile Kernel C files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble Kernel NASM files
%.o: %.S
	$(NASM) $(NASMFLAGS) $< -o $@

# --- User Programs ---

# 1. Assemble entry.S
programs/entry.o: programs/entry.S
	$(NASM) -f elf32 programs/entry.S -o programs/entry.o

# 2. Compile stdlib
programs/stdlib.o: programs/stdlib.c
	$(CC) $(USER_CFLAGS) -c programs/stdlib.c -o programs/stdlib.o

# 3. Compile hello.c
programs/hello.o: programs/hello.c
	$(CC) $(USER_CFLAGS) -c programs/hello.c -o programs/hello.o

# 4. Link hello.elf
hello.elf: $(USER_OBJS) programs/linker.ld
	$(LD) $(USER_LDFLAGS) -o hello.elf programs/entry.o programs/stdlib.o programs/hello.o

# 5. Compile & Link echo.elf
programs/echo.o: programs/echo.c
	$(CC) $(USER_CFLAGS) -c programs/echo.c -o programs/echo.o

echo.elf: programs/echo.o programs/entry.o programs/stdlib.o programs/linker.ld
	$(LD) $(USER_LDFLAGS) -o echo.elf programs/entry.o programs/stdlib.o programs/echo.o

# --- Image Creation ---

# Get Limine (Only clone if not exists)
limine:
	git clone https://github.com/limine-bootloader/limine.git --branch=v8.x-binary --depth=1
	make -C limine

# Create Disk Image (for persistence)
disk.img:
	dd if=/dev/zero of=disk.img bs=1M count=10

# Create ISO
my-os.iso: my-kernel.elf limine hello.elf echo.elf
	rm -rf iso_root
	mkdir -p iso_root
	cp my-kernel.elf iso_root/
	# Create a test text file
	echo "Hello from the filesystem! This text is loaded from disk." > iso_root/test.txt
	# Copy Config and Programs
	cp limine.conf iso_root/
	cp hello.elf iso_root/
	cp echo.elf iso_root/
	# Install Limine
	cp limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-uefi-cd.bin iso_root/
	xorriso -as mkisofs -b limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o my-os.iso
	./limine/limine bios-install my-os.iso

# --- Run ---

run: my-os.iso disk.img
	qemu-system-i386 -cdrom my-os.iso -drive file=disk.img,format=raw,index=0,media=disk -serial stdio -no-reboot -no-shutdown

clean:
	rm -rf src/**/*.o src/kernel/*.o src/cpu/*.o src/drivers/*.o src/mm/*.o
	rm -rf programs/*.o
	rm -rf *.elf *.iso iso_root limine disk.img
