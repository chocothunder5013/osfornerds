#include "process.h"
#include "../mm/heap.h"
#include "../cpu/gdt.h"
#include "fs.h"
#include "../mm/vmm.h"
#include "../gui/window.h"
extern struct file_node *fs_root;
extern void              switch_task(uint32_t *old_esp_ptr, uint32_t new_esp);
extern void              pmm_free_block(void *p);
extern void             *pmm_alloc_block();
extern void              term_print(const char *str);
extern void              tss_set_stack(uint32_t ss, uint32_t esp);
extern void              jump_to_user();
extern void              vmm_free_address_space(page_directory_t *pd);
extern void              set_cr3(uint32_t pd);
extern uint32_t          get_cr3();
extern page_directory_t *vmm_create_address_space();
extern void vmm_map_page_in_dir(page_directory_t *pd, void *phys, void *virt, int flags);
extern void wm_destroy_window(int pid);
process_t  *current_process = 0;
process_t  *ready_queue     = 0;
int         next_pid        = 1;
typedef struct page_node {
    void             *phys_addr;
    void             *virt_addr;
    struct page_node *next;
} page_node_t;
void init_multitasking() {
    current_process                   = (process_t *)kmalloc(sizeof(process_t));
    current_process->pid              = 0;
    current_process->state            = PROCESS_READY;
    current_process->cwd              = fs_root;
    current_process->cr3              = get_cr3();
    current_process->allocated_pages  = 0;
    current_process->kernel_stack_ptr = kmalloc(4096);
    ready_queue                       = current_process;
    current_process->next             = current_process;
    term_print(" [SCHED] Multitasking Initialized.\n");
}
void process_track_page(process_t *proc, void *phys, void *virt) {
    if (!proc)
        return;
    page_node_t *node     = (page_node_t *)kmalloc(sizeof(page_node_t));
    node->phys_addr       = phys;
    node->virt_addr       = virt;
    node->next            = proc->allocated_pages;
    proc->allocated_pages = node;
}
/*
 * Allocates a new process control block (PCB), sets up its initial execution context
 * (including stack and address space), and inserts it into the circular ready queue.
 * For user processes, it constructs an interrupt frame that jump_to_user will
 * pop during an IRET instruction to switch privilege levels.
 */
int create_process(void (*entry_point)(), char *args, uint32_t initial_break, int is_kernel,
                   uint32_t custom_cr3) {
    process_t *new_proc       = (process_t *)kmalloc(sizeof(process_t));
    new_proc->pid             = next_pid++;
    new_proc->parent_pid      = current_process ? current_process->pid : 0;
    new_proc->state           = PROCESS_READY;
    new_proc->cwd             = current_process ? current_process->cwd : fs_root;
    new_proc->program_break   = initial_break;
    new_proc->allocated_pages = 0;
    if (is_kernel) {
        new_proc->cr3 = get_cr3();
    } else {
        if (custom_cr3) {
            new_proc->cr3 = custom_cr3;
        } else {
            page_directory_t *new_pd = vmm_create_address_space();
            new_proc->cr3            = (uint32_t)new_pd;
        }
    }
    new_proc->kernel_stack_ptr = kmalloc(4096);
    uint32_t  ks_top           = (uint32_t)new_proc->kernel_stack_ptr + 4096;
    uint32_t *sp               = (uint32_t *)ks_top;
    if (!is_kernel) {
        void *stack_phys = pmm_alloc_block();
        vmm_map_page_in_dir((page_directory_t *)new_proc->cr3, stack_phys,
                            (void *)(USER_STACK_TOP - 4096), 0x7);
        *(--sp) = 0x23;
        *(--sp) = USER_STACK_TOP;
        *(--sp) = 0x202;
        *(--sp) = 0x1B;
        *(--sp) = (uint32_t)entry_point;
    } else {
        *(--sp) = 0x202;
        *(--sp) = 0x08;
        *(--sp) = (uint32_t)entry_point;
    }
    *(--sp) = (uint32_t)jump_to_user;
    *(--sp) = 0x202;
    *(--sp) = 0x08;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    if (!is_kernel) {
        *(--sp) = 0x23;
        *(--sp) = 0x23;
        *(--sp) = 0x23;
        *(--sp) = 0x23;
    } else {
        *(--sp) = 0x10;
        *(--sp) = 0x10;
        *(--sp) = 0x10;
        *(--sp) = 0x10;
    }
    new_proc->esp   = (uint32_t)sp;
    process_t *last = ready_queue;
    while (last->next != ready_queue)
        last = last->next;
    last->next     = new_proc;
    new_proc->next = ready_queue;
    return new_proc->pid;
}
/*
 * The core round-robin scheduler.
 * Disables interrupts, finds the next process in the ready queue, switches
 * the memory address space (CR3) if necessary, updates the TSS stack pointer
 * for future ring 0 transitions, and performs the CPU context switch.
 */
void schedule() {
    __asm__ volatile("cli");
    if (!current_process) {
        __asm__ volatile("sti");
        return;
    }
    process_t *next_proc = current_process->next;
    while ((next_proc->state != PROCESS_READY) && next_proc != current_process) {
        next_proc = next_proc->next;
    }
    if (next_proc == current_process && next_proc->state != PROCESS_READY) {
        __asm__ volatile("sti");
        __asm__ volatile("hlt");
        return;
    }
    process_t *prev_proc = current_process;
    current_process      = next_proc;
    if (current_process->cr3 != prev_proc->cr3) {
        set_cr3(current_process->cr3);
    }
    tss_set_stack(0x10, (uint32_t)current_process->kernel_stack_ptr + 4096);
    switch_task(&(prev_proc->esp), current_process->esp);
    __asm__ volatile("sti");
}
int get_process_count() {
    if (!ready_queue)
        return 0;
    int        count = 0;
    process_t *node  = ready_queue;
    do {
        if (node->state != PROCESS_ZOMBIE)
            count++;
        node = node->next;
    } while (node != ready_queue);
    return count;
}
/*
 * Terminates the current process, transitions it to a ZOMBIE state,
 * releases its memory space, unblocks the parent waiting for it,
 * destroys its window, and invokes the scheduler to yield the CPU.
 */
void process_exit(int code) {
    __asm__ volatile("cli");
    if (current_process->pid <= 1) {
        __asm__ volatile("sti");
        return;
    }
    kfree(current_process->kernel_stack_ptr);
    if (current_process->cr3 != get_cr3()) {
        vmm_free_address_space((page_directory_t *)current_process->cr3);
    }
    current_process->state     = PROCESS_ZOMBIE;
    current_process->exit_code = code;
    process_unblock(current_process->pid);
    wm_destroy_window(current_process->pid);
    schedule();
}
void process_block(int reason) {
    current_process->state       = PROCESS_BLOCKED;
    current_process->wait_reason = reason;
    schedule();
}
void process_unblock(int reason) {
    if (!ready_queue)
        return;
    process_t *node = ready_queue;
    do {
        if (node->state == PROCESS_BLOCKED && node->wait_reason == reason) {
            node->state       = PROCESS_READY;
            node->wait_reason = 0;
            return;
        }
        node = node->next;
    } while (node != ready_queue);
}
int process_wait(int pid, int *status_ptr) {
    while (1) {
        process_t *child = 0;
        process_t *it    = ready_queue;
        do {
            if (it->pid == pid || (pid == -1 && it->parent_pid == current_process->pid)) {
                child = it;
                break;
            }
            it = it->next;
        } while (it != ready_queue);
        if (!child)
            return -1;
        if (child->state == PROCESS_ZOMBIE) {
            if (status_ptr)
                *status_ptr = child->exit_code;
            process_t *prev = ready_queue;
            while (prev->next != child)
                prev = prev->next;
            prev->next = child->next;
            if (ready_queue == child)
                ready_queue = child->next;
            return child->pid;
        }
        process_block(child->pid);
    }
}
void kill_foreground_process() {
    __asm__ volatile("cli");
    process_t *curr   = ready_queue;
    process_t *target = 0;
    do {
        if (curr->state != PROCESS_ZOMBIE && curr->cr3 != get_cr3()) {
            target = curr;
        }
        curr = curr->next;
    } while (curr != ready_queue);
    if (target) {
        target->state     = PROCESS_ZOMBIE;
        target->exit_code = 130;
        process_unblock(target->pid);
        term_print("\n[OS] Process Terminated via Hardware Ctrl+C Override\n");
    }
    __asm__ volatile("sti");
}
/*
 * The Out-Of-Memory (OOM) killer.
 * Scans the process list to find the process with the highest number of
 * allocated pages and terminates it to free up physical memory.
 * Excludes kernel tasks (those sharing the kernel CR3) from termination.
 */
void invoke_oom_killer() {
    __asm__ volatile("cli");
    if (!ready_queue) {
        __asm__ volatile("sti");
        return;
    }
    process_t *worst_proc = 0;
    int        max_pages  = 0;
    process_t *curr       = ready_queue;
    do {
        if (curr->state != PROCESS_ZOMBIE && curr->cr3 != get_cr3()) {
            int          pages = 0;
            page_node_t *node  = (page_node_t *)curr->allocated_pages;
            while (node) {
                pages++;
                node = node->next;
            }
            if (pages > max_pages) {
                max_pages  = pages;
                worst_proc = curr;
            }
        }
        curr = curr->next;
    } while (curr != ready_queue);
    if (worst_proc) {
        extern window_t *console_win;
        if (console_win)
            console_win->text_color = 0xFFFF0000;
        term_print("\n[OOM KILLER] Critical Memory Exhaustion Detected!\n");
        term_print("[OOM KILLER] Terminating worst offender (PID ");
        char pid_buf[12];
        int  n = worst_proc->pid, i = 10;
        pid_buf[11] = 0;
        if (n == 0) {
            pid_buf[10] = '0';
            i           = 9;
        }
        while (n > 0) {
            pid_buf[i--] = (n % 10) + '0';
            n /= 10;
        }
        term_print(&pid_buf[i + 1]);
        term_print(")...\n");
        if (console_win)
            console_win->text_color = 0xFFFFFFFF;
        if (worst_proc == current_process) {
            __asm__ volatile("sti");
            process_exit(9);
        } else {
            worst_proc->state     = PROCESS_ZOMBIE;
            worst_proc->exit_code = 9;
            process_unblock(worst_proc->pid);
            vmm_free_address_space((page_directory_t *)worst_proc->cr3);
        }
    }
    __asm__ volatile("sti");
}
int kill_process(int pid) {
    __asm__ volatile("cli");
    if (!ready_queue || pid <= 1) {
        __asm__ volatile("sti");
        return -1;
    }
    process_t *curr = ready_queue;
    do {
        if (curr->pid == pid && curr->state != PROCESS_ZOMBIE) {
            curr->state     = PROCESS_ZOMBIE;
            curr->exit_code = 9;
            process_unblock(curr->pid);
            if (curr->cr3 != get_cr3()) {
                vmm_free_address_space((page_directory_t *)curr->cr3);
            }
            wm_destroy_window(pid);
            __asm__ volatile("sti");
            return 0;
        }
        curr = curr->next;
    } while (curr != ready_queue);
    __asm__ volatile("sti");
    return -1;
}