#include "syscall.h"
#include "process.h"
#include "fs.h"
#include "../gui/window.h"
extern window_t  *console_win;
extern void       term_print(const char *str);
extern void       process_exit(int code);
extern void       term_clear();
extern int        sys_open(const char *name);
extern void       sys_close(int fd);
extern int        sys_read_file(int fd, char *buf, int size);
extern int        sys_readdir(int index, char *buf);
extern void       vmm_map_page(void *phys, void *virt, int flags);
extern void       set_cr3(uint32_t pd);
extern void      *pmm_alloc_block();
extern void       fs_delete(const char *name);
extern int        sys_chdir(const char *path);
extern void       sys_getcwd(char *buf, int size);
extern int        sys_write_file(int fd, char *buffer, int size);
extern process_t *current_process;
extern void      *memset(void *ptr, int value, uint32_t num);
extern window_t  *create_window(const char *title, int x, int y, int w, int h);
extern void       wm_print(window_t *win, const char *str);
extern char       wm_pop_key(int pid);
int               is_valid_user_ptr(void *ptr, int size) {
    uint32_t addr = (uint32_t)ptr;
    if (addr + size < addr)
        return 0;
    if (addr + size < addr) return 0;
    return 1;
}
/*
 * Adjusts the process data segment size (program break) by allocating physical pages
 * and mapping them into the current process's virtual address space.
 * It tracks these pages for later cleanup. If physical memory limits are reached,
 * it invokes the OOM killer. Returns the previous break address on success, or -1 on failure.
 */
void *sys_sbrk(int increment) {
    if (!current_process)
        return (void *)-1;
    extern void     invoke_oom_killer();
    extern uint32_t max_blocks;
    extern uint32_t used_blocks;
    if (used_blocks >= max_blocks - 256) {
        invoke_oom_killer();
        return (void *)-1;
    }
    process_t *proc         = current_process;
    uint32_t   old_break    = proc->program_break;
    uint32_t   new_break    = old_break + increment;
    uint32_t   old_page_top = (old_break + 4095) & 0xFFFFF000;
    uint32_t   new_page_top = (new_break + 4095) & 0xFFFFF000;
    if (new_page_top > old_page_top) {
        uint32_t pages_needed = (new_page_top - old_page_top) / 4096;
        for (uint32_t i = 0; i < pages_needed; i++) {
            void *phys = pmm_alloc_block();
            if (!phys)
                return (void *)-1;
            memset(phys, 0, 4096);
            vmm_map_page_in_dir((void *)proc->cr3, phys, (void *)(old_page_top + (i * 4096)), 0x7);
            process_track_page(proc, phys, (void *)(old_page_top + (i * 4096)));
        }
        set_cr3(proc->cr3);
    }
    proc->program_break = new_break;
    return (void *)old_break;
}
/*
 * Demultiplexes software interrupts from user space (int 0x80) to kernel subroutines.
 * The system call number is passed in EAX, with arguments in EBX, ECX, EDX, ESI, and EDI.
 * It validates user pointers to prevent kernel memory corruption and performs the requested action.
 */
void syscall_handler(registers_t *regs) {
    extern process_t *current_process;
    if (current_process && current_process->state == 3) {
        extern void schedule();
        schedule();
        return;
    }
    switch (regs->eax) {
    case SYS_PRINT:
        if (is_valid_user_ptr((void *)regs->ebx, 1))
            term_print((const char *)regs->ebx);
        break;
    case SYS_YIELD:
        schedule();
        break;
    case SYS_READ: {
        extern char wm_pop_key(int pid);
        extern void schedule();
        char        c = 0;
        while ((c = wm_pop_key(current_process->pid)) == 0) {
            schedule();
        }
        regs->eax = (uint32_t)c;
    } break;
    case SYS_EXIT:
        process_exit((int)regs->ebx);
        break;
    case SYS_WAIT:
        regs->eax = process_wait((int)regs->ebx, (int *)regs->ecx);
        break;
    case SYS_OPEN:
        if (is_valid_user_ptr((void *)regs->ebx, 1))
            regs->eax = sys_open((const char *)regs->ebx);
        break;
    case SYS_CLOSE:
        sys_close((int)regs->ebx);
        break;
    case SYS_FREAD:
        if (is_valid_user_ptr((void *)regs->ecx, (int)regs->edx))
            regs->eax = sys_read_file((int)regs->ebx, (char *)regs->ecx, (int)regs->edx);
        break;
    case SYS_READDIR:
        if (is_valid_user_ptr((void *)regs->ecx, 32))
            regs->eax = sys_readdir((int)regs->ebx, (char *)regs->ecx);
        break;
    case SYS_SBRK:
        regs->eax = (uint32_t)sys_sbrk((int)regs->ebx);
        break;
    case SYS_WRITE:
        if (is_valid_user_ptr((void *)regs->ecx, (int)regs->edx))
            regs->eax = sys_write_file((int)regs->ebx, (char *)regs->ecx, (int)regs->edx);
        break;
    case 13:
        term_clear();
        regs->eax = 0;
        break;
    case 14:
        if (is_valid_user_ptr((void *)regs->ebx, 1))
            fs_delete((const char *)regs->ebx);
        regs->eax = 0;
        break;
    case 15:
        if (is_valid_user_ptr((void *)regs->ebx, 1))
            regs->eax = sys_chdir((const char *)regs->ebx);
        break;
    case 16:
        if (is_valid_user_ptr((void *)regs->ebx, (int)regs->ecx))
            sys_getcwd((char *)regs->ebx, (int)regs->ecx);
        break;
    case 17:
        if (is_valid_user_ptr((void *)regs->ebx, sizeof(sysinfo_t))) {
            sysinfo_t      *info = (sysinfo_t *)regs->ebx;
            extern uint32_t system_ticks;
            extern uint32_t max_blocks;
            extern uint32_t used_blocks;
            extern int      get_process_count();
            info->uptime_ticks    = system_ticks;
            info->total_memory_kb = max_blocks * 4;
            info->used_memory_kb  = used_blocks * 4;
            info->process_count   = get_process_count();
            regs->eax             = 0;
        } else {
            regs->eax = -1;
        }
        break;
    case 18:
        if (is_valid_user_ptr((void *)regs->ebx, sizeof(rect_t))) {
            rect_t   *r          = (rect_t *)regs->ebx;
            window_t *target_win = (r->win_id == 0) ? console_win : (window_t *)r->win_id;
            if (target_win) {
                for (int j = 0; j < r->h; j++) {
                    for (int i = 0; i < r->w; i++) {
                        int px = r->x + i;
                        int py = r->y + j;
                        if (px >= 0 && px < target_win->width && py >= 0 &&
                            py < target_win->height) {
                            target_win->buffer[py * target_win->width + px] = r->color;
                        }
                    }
                }
            }
        }
        break;
    case 19:
        if (console_win) {
            console_win->cursor_x = (int)regs->ebx;
            console_win->cursor_y = (int)regs->ecx;
        }
        break;
    case 20:
        if (is_valid_user_ptr((void *)regs->ebx, 1)) {
            window_t *new_win = create_window((const char *)regs->ebx, (int)regs->ecx,
                                              (int)regs->edx, (int)regs->esi, (int)regs->edi);
            regs->eax         = (uint32_t)new_win;
        }
        break;
    case 21:
        if (is_valid_user_ptr((void *)regs->ecx, 1)) {
            window_t *target_win = (regs->ebx == 0) ? console_win : (window_t *)regs->ebx;
            if (target_win) {
                wm_print(target_win, (const char *)regs->ecx);
            }
        }
        break;
    case 22: {
        window_t *target_win = (regs->ebx == 0) ? console_win : (window_t *)regs->ebx;
        if (target_win) {
            target_win->cursor_x = (int)regs->ecx;
            target_win->cursor_y = (int)regs->edx;
        }
    } break;
    case 23: {
        window_t *target_win = (regs->ebx == 0) ? console_win : (window_t *)regs->ebx;
        if (target_win) {
            target_win->text_color = (uint32_t)regs->ecx;
        }
    } break;
    case 24: {
        extern int kill_process(int pid);
        regs->eax = (uint32_t)kill_process((int)regs->ebx);
    } break;
    }
}