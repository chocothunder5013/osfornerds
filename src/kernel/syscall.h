#ifndef SYSCALL_H
#define SYSCALL_H
#include <stdint.h>
#include "../cpu/idt.h"

/*
 * System Call Number Definitions.
 * These constants are passed in the EAX register by user programs to identify
 * the requested service.
 */
#define SYS_PRINT 0
#define SYS_YIELD 1
#define SYS_READ 2
#define SYS_EXIT 3
#define SYS_WAIT 4
#define SYS_OPEN 5
#define SYS_CLOSE 6
#define SYS_FREAD 7
#define SYS_READDIR 8
#define SYS_SBRK 9
#define SYS_WRITE 11
#define SYS_SEEK 12
#define SYS_IOCTL 13
#define SYS_SYSINFO 17
#define SYS_DRAW_RECT 18
#define SYS_SET_CURSOR 19
#define SYS_CREATE_WINDOW 20
#define SYS_WIN_PRINT 21
#define SYS_KILL 24

/*
 * Structure for window rendering system calls.
 */
typedef struct {
    int      win_id;
    int      x, y, w, h;
    uint32_t color;
} rect_t;

/*
 * Structure used by SYS_SYSINFO to retrieve global system status.
 */
typedef struct {
    uint32_t uptime_ticks;
    uint32_t total_memory_kb;
    uint32_t used_memory_kb;
    uint32_t process_count;
} sysinfo_t;
void syscall_handler(registers_t *regs);
void init_syscalls();
#endif