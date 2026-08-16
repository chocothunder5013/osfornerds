import os
import sys
import struct
import math

# Magic number identifying our custom file system format.
FS_MAGIC = 0x1337BEEF
SECTOR_SIZE = 512

if len(sys.argv) < 3:
    print("Usage: python build_fs.py <output_disk.img> <file1> <file2> ...")
    sys.exit(1)

# The first argument is the output disk image, followed by the files to pack.
disk_file = sys.argv[1]
elf_files = sys.argv[2:]

with open(disk_file, 'wb') as f:
    # Write the superblock at sector 0. It includes our magic number and file count.
    f.write(struct.pack('<I I', FS_MAGIC, len(elf_files)))
    
    # Pad the rest of sector 0 with zeros to complete the 512-byte block.
    f.write(b'\x00' * (SECTOR_SIZE - 8))
    
    table_bytes = bytearray()
    current_data_sector_offset = 0
    file_payloads = []
    
    # Build the file metadata table and read file payloads into memory.
    for elf in elf_files:
        name = os.path.basename(elf).encode('ascii')[:63]
        with open(elf, 'rb') as e:
            data = e.read()
        
        size = len(data)
        
        # Append metadata record: 64-byte name, 4-byte size, 4-byte sector offset.
        table_bytes += struct.pack('<64s I I', name, size, current_data_sector_offset)
        
        # Calculate how many sectors this file requires and pad the payload.
        sectors_needed = math.ceil(size / SECTOR_SIZE)
        padded_data = data + b'\x00' * ((sectors_needed * SECTOR_SIZE) - size)
        
        file_payloads.append(padded_data)
        current_data_sector_offset += sectors_needed
    
    # The metadata table takes up 8 sectors (4096 bytes). Pad it if it is smaller.
    if len(table_bytes) < 4096:
        table_bytes += b'\x00' * (4096 - len(table_bytes))
    f.write(table_bytes[:4096])
    
    # Sector 9 is reserved as padding. Write 512 empty bytes.
    f.write(b'\x00' * SECTOR_SIZE)
    
    # Starting at sector 10, write all the contiguous file payloads.
    for payload in file_payloads:
        f.write(payload)
        
    # Pad the entire disk image to exactly 10MB to satisfy emulator constraints.
    current_size = f.tell()
    target_size = 10 * 1024 * 1024
    if current_size < target_size:
        f.write(b'\x00' * (target_size - current_size))

print(f"Successfully packed {len(elf_files)} files into {disk_file}")
