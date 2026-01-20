/* src/kernel/main.c */
#include <stdint.h>
#include "multiboot.h"
#include "process.h"
#include "../cpu/gdt.h"
#include "../cpu/idt.h"
#include "../mm/heap.h"
#include "syscall.h"
#include "../gui/window.h"

// --- Externs ---
extern void init_serial();
extern void serial_log(char *str);
extern void serial_write_char(char c);

extern void init_pmm(uint32_t mem_size);
extern void init_vmm();
extern void init_heap();

extern void init_fs(multiboot_info_t* mboot_ptr);
extern void execute_command(char* input); 

// Graphics & Inputs
extern void init_graphics(multiboot_info_t* mboot);
extern void init_ata();
extern void init_mouse();
extern int mouse_x;
extern int mouse_y;
extern int mouse_left_button;

extern void wm_putc(window_t* win, char c);
extern void wm_print(window_t* win, const char* str);
extern int strlen(const char* str);
extern void strcpy_safe(char* dest, const char* src);

window_t* console_win = 0;

// --- Syscall Wrappers ---
void sys_print(const char* msg) {
    __asm__ volatile ("int $0x80" : : "a" (SYS_PRINT), "b" (msg));
}
void sys_yield() {
    __asm__ volatile ("int $0x80" : : "a" (SYS_YIELD));
}
char sys_read() {
    char c;
    __asm__ volatile ("int $0x80" : "=a" (c) : "a" (SYS_READ));
    return c;
}

// --- Terminal Abstraction ---

void term_putc(char c) {
    if (console_win) {
        wm_putc(console_win, c);
    } else {
        serial_write_char(c);
    }
}

void term_print(const char* str) {
    if (console_win) {
        wm_print(console_win, str);
    } else {
        serial_log((char*)str);
    }
}

void term_clear() {
    if (console_win) {
        // Clear window buffer to bg_color
        int total = console_win->width * console_win->height;
        for(int i = 0; i < total; i++) {
            console_win->buffer[i] = console_win->bg_color;
        }
        console_win->cursor_x = 0;
        console_win->cursor_y = 0;
    }
}

// --- Shell & Tasks ---

#define HISTORY_MAX 10
#define CMD_LEN 256
char history[HISTORY_MAX][CMD_LEN];
int history_count = 0;

void shell_task() {
    char cmd_buffer[CMD_LEN];
    int idx = 0;
    int history_view_idx = 0; 

    // MODIFIED: Message indicates Kernel Mode
    sys_print("\n[SHELL] Process Started (Kernel Mode).\n");
    sys_print("MyOS Shell > ");

    while(1) {
        char c = sys_read();
        
        if (c == 0) {
            sys_yield();
            continue;
        }

        // UP ARROW (0x11)
        if (c == 0x11) {
            if (history_count > 0 && history_view_idx > 0) {
                while(idx > 0) { sys_print("\b \b"); idx--; }
                history_view_idx--;
                strcpy_safe(cmd_buffer, history[history_view_idx]);
                idx = strlen(cmd_buffer);
                sys_print(cmd_buffer);
            }
        }
        // DOWN ARROW (0x12)
        else if (c == 0x12) {
            if (history_count > 0 && history_view_idx < history_count) {
                while(idx > 0) { sys_print("\b \b"); idx--; }
                history_view_idx++;
                if (history_view_idx == history_count) {
                    cmd_buffer[0] = 0; idx = 0;
                } else {
                    strcpy_safe(cmd_buffer, history[history_view_idx]);
                    idx = strlen(cmd_buffer);
                    sys_print(cmd_buffer);
                }
            }
        }
        // ENTER
        else if (c == '\n') {
            sys_print("\n");
            cmd_buffer[idx] = '\0';
            
            if (idx > 0) {
                if (history_count < HISTORY_MAX) {
                    strcpy_safe(history[history_count], cmd_buffer);
                    history_count++;
                } else {
                    for(int i=0; i<HISTORY_MAX-1; i++) strcpy_safe(history[i], history[i+1]);
                    strcpy_safe(history[HISTORY_MAX-1], cmd_buffer);
                }
                history_view_idx = history_count; 
                execute_command(cmd_buffer);
            } else {
                execute_command(""); 
            }
            idx = 0;
        }
        // BACKSPACE
        else if (c == '\b') {
            if (idx > 0) { idx--; sys_print("\b \b"); }
        }
        // NORMAL CHAR
        else if (c >= ' ' && c <= '~') {
            if (idx < CMD_LEN - 1) {
                cmd_buffer[idx++] = c;
                char temp[2] = {c, '\0'};
                sys_print(temp);
            }
        }
    }
}

void system_monitor_task() {
    int counter = 0;
    while(1) {
        counter++;
        if (counter % 100000000 == 0) {
            serial_log(" [BG-TASK] System Alive. Tick...\n");
            counter = 0;
        }
    }
}

void kmain(multiboot_info_t* mboot_ptr) {
    init_serial();
    
    // 1. Initialize Core
    init_gdt();
    init_idt();
    init_pmm(mboot_ptr->mem_upper); 
    init_vmm();
    init_heap();
    init_ata();
    init_fs(mboot_ptr);

    // 2. Start GUI
    init_graphics(mboot_ptr);
    init_mouse();
    init_wm();
    
    // Create the Terminal Window
    console_win = create_window("Terminal", 50, 50, 600, 400);
    console_win->text_color = 0xFFFFFFFF;
    console_win->bg_color   = 0xFF000000;
    // Clear buffer to black
    for(int i=0; i < 600*400; i++) console_win->buffer[i] = 0xFF000000;

    term_print(" [SYSTEM] Graphics Terminal Initialized.\n");

    // 3. Start Multitasking
    init_multitasking();
    
    // FIX: Pass '1' as the last argument to create Kernel Threads (Ring 0).
    // This allows them to access the kernel heap (console_win) and I/O ports without crashing.
    create_process(system_monitor_task, 0, 0, 1);
    create_process(shell_task, 0, 0, 1);

    // 4. Main Loop (The Compositor)
    while(1) {
        // We handle mouse state updates here before refreshing
        wm_mouse_event(mouse_x, mouse_y, mouse_left_button);
        
        // Draw the desktop
        wm_refresh();
    }
}