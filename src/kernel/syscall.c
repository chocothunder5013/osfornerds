/* src/kernel/syscall.c */
#include "syscall.h"
#include "process.h"
#include "fs.h" 

extern void term_print(const char* str); 
extern void process_exit(int code);
extern char kbd_buffer_read();
extern void term_clear();
extern int sys_open(const char* name);
extern void sys_close(int fd);
extern int sys_read_file(int fd, char* buf, int size);
extern int sys_readdir(int index, char* buf);
extern void vmm_map_page(void* phys, void* virt, int flags);
extern void* pmm_alloc_block();
extern void fs_delete(const char* name);
extern int sys_chdir(const char* path);
extern void sys_getcwd(char* buf, int size);
extern int sys_write_file(int fd, char* buffer, int size);
extern process_t* current_process;
extern void* memset(void* ptr, int value, uint32_t num); 

// Security Check
// Ensure the pointer + size is NOT within Kernel Space (0 - 128MB).
int is_valid_user_ptr(void* ptr, int size) {
    uint32_t addr = (uint32_t)ptr;
    // Check for overflow
    if (addr + size < addr) return 0;
    // Protect Kernel Low Mem (0 - 128MB)
    // The kernel is identity mapped here. Accessing this from user mode 
    // without the proper flags will cause a page fault (which is good),
    // but we return 0 here to fail the syscall gracefully instead of crashing the process.
    if (addr < 0x08000000) return 0;
    if (addr + size < 0x08000000) return 0;

    return 1;
}

void* sys_sbrk(int increment) {
    if (!current_process) return (void*)-1;
    process_t* proc = current_process;
    uint32_t old_break = proc->program_break;
    uint32_t new_break = old_break + increment;
    
    // Page align (4KB)
    uint32_t old_page_top = (old_break + 4095) & 0xFFFFF000;
    uint32_t new_page_top = (new_break + 4095) & 0xFFFFF000;

    if (new_page_top > old_page_top) {
        uint32_t pages_needed = (new_page_top - old_page_top) / 4096;
        for (uint32_t i = 0; i < pages_needed; i++) {
            void* phys = pmm_alloc_block();
            if (!phys) return (void*)-1;
            
            // SECURITY FIX: Zero out the new memory to prevent leaking kernel data
            memset(phys, 0, 4096);

            vmm_map_page(phys, (void*)(old_page_top + (i * 4096)), 0x7);
            process_track_page(proc, phys, (void*)(old_page_top + (i * 4096)));
        }
    }
    proc->program_break = new_break;
    return (void*)old_break;
}

void syscall_handler(registers_t* regs) {
    switch (regs->eax) {
        case SYS_PRINT: 
            if (is_valid_user_ptr((void*)regs->ebx, 1)) 
                term_print((const char*)regs->ebx);
            break;
            
        case SYS_YIELD: schedule(); break;
        
        case SYS_READ: {
            __asm__ volatile("cli");
            char c = kbd_buffer_read();
            if (c != 0) {
                __asm__ volatile("sti");
                regs->eax = (uint32_t)c;
            } else {
                process_block(1);
                regs->eax = 0;
                __asm__ volatile("sti");
            }
            break;
        }
        
        case SYS_EXIT: process_exit((int)regs->ebx); break;
        case SYS_WAIT: regs->eax = process_wait((int)regs->ebx, (int*)regs->ecx); break;
        
        case SYS_OPEN: 
            if (is_valid_user_ptr((void*)regs->ebx, 1))
                regs->eax = sys_open((const char*)regs->ebx);
            break;
            
        case SYS_CLOSE: sys_close((int)regs->ebx); break;
        
        case SYS_FREAD: 
            if (is_valid_user_ptr((void*)regs->ecx, (int)regs->edx))
                regs->eax = sys_read_file((int)regs->ebx, (char*)regs->ecx, (int)regs->edx);
            break;
            
        case SYS_READDIR: 
             if (is_valid_user_ptr((void*)regs->ecx, 32))
                regs->eax = sys_readdir((int)regs->ebx, (char*)regs->ecx);
            break;
             
        case SYS_SBRK: regs->eax = (uint32_t)sys_sbrk((int)regs->ebx); break;
        
        case SYS_WRITE: 
            if (is_valid_user_ptr((void*)regs->ecx, (int)regs->edx))
                regs->eax = sys_write_file((int)regs->ebx, (char*)regs->ecx, (int)regs->edx);
            break;
        
        case 13: term_clear(); regs->eax = 0; break;
        case 14: 
            if (is_valid_user_ptr((void*)regs->ebx, 1))
                fs_delete((const char*)regs->ebx);
            regs->eax = 0; 
            break;
            
        case 15: 
            if (is_valid_user_ptr((void*)regs->ebx, 1))
                regs->eax = sys_chdir((const char*)regs->ebx);
            break;
        case 16: 
            if (is_valid_user_ptr((void*)regs->ebx, (int)regs->ecx))
                sys_getcwd((char*)regs->ebx, (int)regs->ecx);
            break;
    }
}