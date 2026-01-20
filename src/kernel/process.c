/* src/kernel/process.c */
#include "process.h"
#include "../mm/heap.h"
#include "../cpu/gdt.h"
#include "fs.h"
#include "../mm/vmm.h"

extern struct file_node* fs_root;
extern void switch_task(uint32_t *old_esp_ptr, uint32_t new_esp);
extern void pmm_free_block(void* p);
extern void* pmm_alloc_block();
extern void term_print(const char* str);
extern void tss_set_stack(uint32_t ss, uint32_t esp);
extern void jump_to_user();
extern void vmm_free_address_space(page_directory_t* pd);
extern void set_cr3(uint32_t pd);
extern uint32_t get_cr3();

extern page_directory_t* vmm_create_address_space();
extern void vmm_map_page_in_dir(page_directory_t* pd, void* phys, void* virt, int flags);

process_t* current_process = 0;
process_t* ready_queue = 0;
int next_pid = 1;

typedef struct page_node {
    void* phys_addr;
    void* virt_addr;
    struct page_node* next;
} page_node_t;

void init_multitasking() {
    current_process = (process_t*)kmalloc(sizeof(process_t));
    current_process->pid = 0;
    current_process->state = PROCESS_READY;
    current_process->cwd = fs_root;
    current_process->cr3 = get_cr3(); 
    current_process->allocated_pages = 0;
    
    current_process->kernel_stack_ptr = kmalloc(4096);
    
    ready_queue = current_process;
    current_process->next = current_process; 

    term_print(" [SCHED] Multitasking Initialized.\n");
}

void process_track_page(process_t* proc, void* phys, void* virt) {
    if (!proc) return;
    page_node_t* node = (page_node_t*)kmalloc(sizeof(page_node_t));
    node->phys_addr = phys;
    node->virt_addr = virt;
    node->next = proc->allocated_pages;
    proc->allocated_pages = node;
}

// UPDATED: Handles is_kernel flag
int create_process(void (*entry_point)(), char* args, uint32_t initial_break, int is_kernel) {
    process_t* new_proc = (process_t*)kmalloc(sizeof(process_t));
    
    new_proc->pid = next_pid++;
    new_proc->parent_pid = current_process ? current_process->pid : 0;
    new_proc->state = PROCESS_READY;
    new_proc->cwd = current_process ? current_process->cwd : fs_root;
    new_proc->program_break = initial_break;
    new_proc->allocated_pages = 0;

    // 1. Setup Address Space
    // Kernel threads share the kernel directory. User processes get a new one.
    if (is_kernel) {
        new_proc->cr3 = get_cr3(); // Reuse current kernel directory
    } else {
        page_directory_t* new_pd = vmm_create_address_space();
        new_proc->cr3 = (uint32_t)new_pd;
    }

    // 2. Allocate Kernel Stack
    new_proc->kernel_stack_ptr = kmalloc(4096);
    uint32_t ks_top = (uint32_t)new_proc->kernel_stack_ptr + 4096;
    uint32_t* sp = (uint32_t*)ks_top;

    // 3. User Stack (Only for User Processes)
    if (!is_kernel) {
        void* stack_phys = pmm_alloc_block();
        // Map User Stack (0x7 = User | RW)
        vmm_map_page_in_dir((page_directory_t*)new_proc->cr3, stack_phys, (void*)(USER_STACK_TOP - 4096), 0x7);
        
        // --- RING 3 IRET FRAME (5 Values) ---
        *(--sp) = 0x23;             // SS (User Data)
        *(--sp) = USER_STACK_TOP;   // ESP (User Stack)
        *(--sp) = 0x202;            // EFLAGS (Interrupts Enabled)
        *(--sp) = 0x1B;             // CS (User Code)
        *(--sp) = (uint32_t)entry_point; // EIP
    } else {
        // --- RING 0 IRET FRAME (3 Values) ---
        // IRET only pops CS/EIP/EFLAGS when returning to the same privilege level
        *(--sp) = 0x202;            // EFLAGS
        *(--sp) = 0x08;             // CS (Kernel Code)
        *(--sp) = (uint32_t)entry_point; // EIP
    }

    // B. The switch_task Frame
    *(--sp) = (uint32_t)jump_to_user; // Return Address (trampoline)

    *(--sp) = 0x202;    // EFLAGS
    *(--sp) = 0x08;     // CS
    *(--sp) = 0;        // Error Code

    // Registers
    *(--sp) = 0; // EDI
    *(--sp) = 0; // ESI
    *(--sp) = 0; // EBP
    *(--sp) = 0; // ESP
    *(--sp) = 0; // EBX
    *(--sp) = 0; // EDX
    *(--sp) = 0; // ECX
    *(--sp) = 0; // EAX

    // Segments
    *(--sp) = 0x10; // DS (Kernel Data)
    *(--sp) = 0x10; // ES
    *(--sp) = 0x10; // FS
    *(--sp) = 0x10; // GS

    new_proc->esp = (uint32_t)sp;
    
    // Add to Ready Queue
    process_t* last = ready_queue;
    while (last->next != ready_queue) last = last->next;
    last->next = new_proc;
    new_proc->next = ready_queue;
    
    return new_proc->pid;
}

void schedule() {
    __asm__ volatile("cli");
    
    if (!current_process) { __asm__ volatile("sti"); return; }

    process_t* next_proc = current_process->next;
    
    while ((next_proc->state != PROCESS_READY) && next_proc != current_process) {
        next_proc = next_proc->next;
    }

    if (next_proc == current_process && next_proc->state != PROCESS_READY) {
        __asm__ volatile("sti");
        __asm__ volatile("hlt"); 
        return;
    }

    process_t* prev_proc = current_process;
    current_process = next_proc;
    
    if (current_process->cr3 != prev_proc->cr3) {
        set_cr3(current_process->cr3);
    }
    
    tss_set_stack(0x10, (uint32_t)current_process->kernel_stack_ptr + 4096);
    switch_task(&(prev_proc->esp), current_process->esp);
    __asm__ volatile("sti");
}

void process_exit(int code) {
    __asm__ volatile("cli");
    
    // Prevent killing init (pid 0) or shell (pid 1) carelessly for now
    if (current_process->pid <= 1) { 
        __asm__ volatile("sti"); 
        return;
    } 

    kfree(current_process->kernel_stack_ptr);
    
    // FIX: Free the address space!
    // We can't free the CURRENT directory while we are using it.
    // However, since we are becoming a ZOMBIE, we will switch away shortly.
    // Real OSs free this in the reaper/wait function.
    // For simplicity in this OS: We mark it, and the PARENT or SCHEDULER should free it.
    // BUT, since we don't have a complex reaper, let's free the USER PAGES now, 
    // but keep the directory struct until the task is fully removed from queue.
    
    // Actually, safest approach for this OS structure:
    // Only free if we are not sharing the kernel directory (is_kernel checks)
    if (current_process->cr3 != get_cr3()) { 
         // If we are running on this CR3, we must be careful. 
         // Usually, we switch to kernel_cr3 before dying.
         // Let's assume the Scheduler handles the CR3 switch.
         
         // Helper: We simply flag it. The actual free should happen when the parent waits.
         // But to stop the leak NOW without complex logic:
         vmm_free_address_space((page_directory_t*)current_process->cr3);
    }
    
    current_process->state = PROCESS_ZOMBIE;
    current_process->exit_code = code;
    
    process_unblock(current_process->pid); 
    
    schedule();
}

void process_block(int reason) {
    current_process->state = PROCESS_BLOCKED;
    current_process->wait_reason = reason;
    schedule();
}

void process_unblock(int reason) {
    process_t* node = ready_queue;
    do {
        if (node->state == PROCESS_BLOCKED && node->wait_reason == reason) {
            node->state = PROCESS_READY;
            node->wait_reason = 0;
            return; 
        }
        node = node->next;
    } while (node != ready_queue);
}

int process_wait(int pid, int* status_ptr) {
    while(1) {
        process_t* child = 0;
        process_t* it = ready_queue;
        do {
            if (it->pid == pid || (pid == -1 && it->parent_pid == current_process->pid)) {
                child = it;
                break;
            }
            it = it->next;
        } while (it != ready_queue);
        
        if (!child) return -1; 
        
        if (child->state == PROCESS_ZOMBIE) {
            if (status_ptr) *status_ptr = child->exit_code;
            
            process_t* prev = ready_queue;
            while (prev->next != child) prev = prev->next;
            prev->next = child->next;
            if (ready_queue == child) ready_queue = child->next;
            
            return child->pid;
        }
        process_block(child->pid); 
    }
}