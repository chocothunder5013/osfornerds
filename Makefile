# Makefile - Final "Deep Tech" Version (Automated Userland)
# Ensure $HOME/opt/cross/bin is in your PATH if using a cross-compiler!

CC = gcc
NASM = nasm 

# --- Compiler Flags ---
# Anti-PIE flags added for modern GCC compatibility
CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -Isrc -mgeneral-regs-only -fno-pic -fno-pie
LDFLAGS = -m32 -T linker.ld -ffreestanding -O2 -nostdlib -no-pie -Wl,--build-id=none -lgcc
NASMFLAGS = -f elf32

USER_CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -Iprograms -mgeneral-regs-only -fno-pic -fno-pie
USER_LDFLAGS = -m32 -T programs/linker.ld -ffreestanding -O2 -nostdlib -no-pie -lgcc

# --- Source Files ---
# Kernel Sources
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

# --- Dynamic User Programs ---
# List the base names of your userland apps here
USER_APPS = cat date echo hello kedit ls memtest
# Automatically generate the .elf target names
USER_ELFS = $(USER_APPS:=.elf)

# Base user libraries every app needs
USER_LIBS = programs/entry.o programs/stdlib.o

# --- Main Targets ---

all: my-os.iso

# Link the kernel (Switched to CC for libgcc inclusion)
my-kernel.elf: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

# Compile Kernel C files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble Kernel NASM files
%.o: %.S
	$(NASM) $(NASMFLAGS) $< -o $@

# --- Userland Rules ---

# Compile any userland C file into an object file
programs/%.o: programs/%.c
	$(CC) $(USER_CFLAGS) -c $< -o $@

# Assemble entry.S
programs/entry.o: programs/entry.S
	$(NASM) -f elf32 $< -o $@

# Static Pattern Rule: Link every .elf file using its corresponding .o file and the base libraries
$(USER_ELFS): %.elf: programs/%.o $(USER_LIBS) programs/linker.ld
	$(CC) $(USER_LDFLAGS) -o $@ $(USER_LIBS) programs/$*.o

# --- Image Creation ---

limine:
	git clone https://github.com/limine-bootloader/limine.git --branch=v8.x-binary --depth=1
	make -C limine

disk.img:
	dd if=/dev/zero of=disk.img bs=1M count=10

# Note: Added $(USER_ELFS) as a dependency so they all build before the ISO
my-os.iso: my-kernel.elf limine $(USER_ELFS) disk.img
	rm -rf iso_root
	mkdir -p iso_root
	cp my-kernel.elf iso_root/
	echo "Hello from the filesystem! This text is loaded from disk." > iso_root/test.txt
	cp limine.conf iso_root/
	# Copy all compiled user programs to the ISO
	cp $(USER_ELFS) iso_root/
	cp limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-uefi-cd.bin iso_root/
	xorriso -as mkisofs -b limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o my-os.iso
	./limine/limine bios-install my-os.iso

# --- Run ---

run: my-os.iso
	qemu-system-i386 -cdrom my-os.iso -drive file=disk.img,format=raw,index=0,media=disk -serial file:serial.log -d int,cpu_reset -D qemu.log

clean:
	rm -rf src/**/*.o src/kernel/*.o src/cpu/*.o src/drivers/*.o src/mm/*.o
	rm -rf programs/*.o
	rm -rf *.elf *.iso iso_root limine disk.img qemu.log serial.log