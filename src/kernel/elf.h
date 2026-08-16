#ifndef ELF_H
#define ELF_H
#include <stdint.h>
#define ELF_MAGIC 0x464C457F
#define PT_LOAD 1
/*
 * The standard 32-bit ELF header.
 * Defines the file format, architecture, and provides offsets to the program
 * and section header tables. The 'entry' field specifies the first instruction to execute.
 */
typedef struct {
    uint32_t magic;
    uint8_t class;
    uint8_t  endian;
    uint8_t  hdr_version;
    uint8_t  abi;
    uint8_t  abi_version;
    uint8_t  pad[7];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint32_t entry;
    uint32_t phoff;
    uint32_t shoff;
    uint32_t flags;
    uint16_t hsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
} elf_header_t;

/*
 * Describes a segment in the ELF file that needs to be loaded into memory.
 * For executable files, PT_LOAD segments tell the OS where in virtual memory (vaddr)
 * to copy the segment data, and how much space it requires (memsz vs filesz).
 */
typedef struct {
    uint32_t type;
    uint32_t offset;
    uint32_t vaddr;
    uint32_t paddr;
    uint32_t filesz;
    uint32_t memsz;
    uint32_t flags;
    uint32_t align;
} elf_program_header_t;
int elf_load_file(const char *filename, char *args);
#endif