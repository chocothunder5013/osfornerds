/* src/kernel/process.h */
#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include "../mm/vmm.h" 

#define MAX_OPEN_FILES 16

// Process States
#define PROCESS_READY   0
#define PROCESS_BLOCKED 1
#define PROCESS_ZOMBIE  2 

// Constants
#define USER_STACK_TOP  0xBFFFF000 
#define USER_STACK_SIZE 0x4000     

struct file_node;

typedef struct {
    struct file_node* file_node; 
    int offset;
    int flags;       
} file_descriptor_t;

typedef struct process {
    int pid;
    int parent_pid;
    int state;
    int wait_reason;
    int exit_code;
    
    uint32_t esp;             
    uint32_t cr3;             
    void* kernel_stack_ptr;   
    
    uint32_t program_break;   
    struct page_node* allocated_pages; 
    
    struct file_node* cwd;
    file_descriptor_t fd_table[MAX_OPEN_FILES];

    struct process *next;
} process_t;

// API
void init_multitasking();

// NEW: Added 'is_kernel' parameter
int create_process(void (*entry_point)(), char* args, uint32_t initial_break, int is_kernel);

void process_exit(int code);
void schedule();
void process_block(int reason);
void process_unblock(int reason);
int process_wait(int pid, int* status);
void process_track_page(process_t* proc, void* phys, void* virt);

#endif