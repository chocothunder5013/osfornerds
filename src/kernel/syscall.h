/* src/kernel/syscall.h */
#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include "../cpu/idt.h" // For registers_t

// Syscall ID numbers

#define SYS_PRINT 0
#define SYS_YIELD 1
#define SYS_READ  2
#define SYS_EXIT  3
#define SYS_WAIT  4
#define SYS_SBRK  9
#define SYS_OPEN    5
#define SYS_CLOSE   6
#define SYS_FREAD   7  // "FREAD" to avoid conflict with keyboard READ
#define SYS_READDIR 8
#define SYS_WRITE 11
#define SYS_SEEK 12
#define SYS_IOCTL 13

// The dispatcher function called by the Interrupt Handler
void syscall_handler(registers_t* regs);

// Initialization
void init_syscalls();

#endif