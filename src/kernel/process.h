#ifndef PROCESS_H
#define PROCESS_H
#include <stdint.h>
#include "../mm/vmm.h"
#define MAX_OPEN_FILES 16
#define PROCESS_READY 0
#define PROCESS_BLOCKED 1
#define PROCESS_ZOMBIE 2
#define USER_STACK_TOP 0xBFFFF000
#define USER_STACK_SIZE 0x4000
struct file_node;
typedef struct {
    struct file_node *file_node;
    int               offset;
    int               flags;
} file_descriptor_t;
/*
 * Process Control Block (PCB) representing a scheduled task.
 * Contains the process ID, state, saved registers (like ESP and CR3),
 * memory allocation tracking, file descriptor table, and links for the circular queue.
 */
typedef struct process {
    int               pid;
    int               parent_pid;
    int               state;
    int               wait_reason;
    int               exit_code;
    uint32_t          esp;
    uint32_t          cr3;
    void             *kernel_stack_ptr;
    uint32_t          program_break;
    struct page_node *allocated_pages;
    struct file_node *cwd;
    file_descriptor_t fd_table[MAX_OPEN_FILES];
    struct process   *next;
} process_t;
void init_multitasking();
int  create_process(void (*entry_point)(), char *args, uint32_t initial_break, int is_kernel,
                    uint32_t custom_cr3);
void process_exit(int code);
void schedule();
void process_block(int reason);
void process_unblock(int reason);
int  process_wait(int pid, int *status);
void process_track_page(process_t *proc, void *phys, void *virt);
#endif