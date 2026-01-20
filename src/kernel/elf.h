#ifndef ELF_H
#define ELF_H

#include <stdint.h>

// ELF Magic Numbers
#define ELF_MAGIC 0x464C457F  // "\x7FELF" in little endian

// Program Header Type
#define PT_LOAD 1

typedef struct {
    uint32_t magic;      // 0x7F 'E' 'L' 'F'
    uint8_t  class;      // 1 = 32-bit
    uint8_t  endian;     // 1 = Little Endian
    uint8_t  hdr_version;// Header Version
    uint8_t  abi;        // 0 = System V
    uint8_t  abi_version;
    uint8_t  pad[7];
    uint16_t type;       // 2 = Executable
    uint16_t machine;    // 3 = x86
    uint32_t version;
    uint32_t entry;      // Entry point address
    uint32_t phoff;      // Program Header Offset
    uint32_t shoff;      // Section Header Offset
    uint32_t flags;
    uint16_t hsize;      // Header Size
    uint16_t phentsize;  // Program Header Entry Size
    uint16_t phnum;      // Program Header Count
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
} elf_header_t;

typedef struct {
    uint32_t type;       // 1 = LOAD
    uint32_t offset;     // Offset in file
    uint32_t vaddr;      // Virtual Address in memory
    uint32_t paddr;
    uint32_t filesz;     // Size in file
    uint32_t memsz;      // Size in memory (if memsz > filesz, zero fill the difference)
    uint32_t flags;      // R/W/X
    uint32_t align;
} elf_program_header_t;

// Function prototype
int elf_load_file(const char* filename, char* args);

#endif