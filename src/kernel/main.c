#include <stdint.h>
#include "multiboot.h"
#include "process.h"
#include "../cpu/gdt.h"
#include "../cpu/idt.h"
#include "../mm/heap.h"
#include "syscall.h"
#include "../gui/window.h"
extern void init_serial();
extern void serial_log(char *str);
extern void serial_write_char(char c);
extern void sys_getcwd(char *buf, int size);
extern int  sys_readdir(int index, char *buf);
extern void init_pmm(uint32_t mem_size);
extern void init_vmm();
extern void init_heap();
extern void init_fs(multiboot_info_t *mboot_ptr);
extern void execute_command(char *input);
extern void init_graphics(multiboot_info_t *mboot);
extern void init_ata();
extern void init_mouse();
extern int  mouse_x;
extern int  mouse_y;
extern int  mouse_left_button;
extern void wm_putc(window_t *win, char c);
extern void wm_print(window_t *win, const char *str);
extern int  strlen(const char *str);
extern void strcpy_safe(char *dest, const char *src);
window_t   *console_win = 0;
/*
 * Triggers a system call to print a null-terminated string.
 * This function loads the system call number into EAX and the string pointer into EBX,
 * then triggers a software interrupt to transition to kernel mode.
 */
void sys_print(const char *msg) {
    __asm__ volatile("int $0x80" : : "a"(SYS_PRINT), "b"(msg) : "memory");
}
void sys_yield() {
    __asm__ volatile("int $0x80" : : "a"(SYS_YIELD));
}
char sys_read() {
    char c;
    __asm__ volatile("int $0x80" : "=a"(c) : "a"(SYS_READ));
    return c;
}
void term_putc(char c) {
    if (console_win) {
        wm_putc(console_win, c);
    } else {
        serial_write_char(c);
    }
}
void term_print(const char *str) {
    if (console_win) {
        wm_print(console_win, str);
    } else {
        serial_log((char *)str);
    }
}
void term_clear() {
    if (console_win) {
        int total = console_win->width * console_win->height;
        for (int i = 0; i < total; i++) {
            console_win->buffer[i] = console_win->bg_color;
        }
        console_win->cursor_x = 0;
        console_win->cursor_y = 0;
    }
}
#define HISTORY_MAX 10
#define CMD_LEN 256
char history[HISTORY_MAX][CMD_LEN];
int  history_count = 0;
void print_prompt() {
    char cwd[64];
    sys_getcwd(cwd, 64);
    sys_print("\nroot@OSForNerds:[");
    sys_print(cwd);
    sys_print("]# ");
}
void shell_task() {
    char cmd_buffer[CMD_LEN];
    int  idx              = 0;
    int  history_view_idx = 0;
    sys_print("\n[SHELL] Modern Terminal Started.\n");
    print_prompt();
    while (1) {
        char c = sys_read();
        if (c == 0) {
            sys_yield();
            continue;
        }
        if (c == 0x11) {
            if (history_count > 0 && history_view_idx > 0) {
                while (idx > 0) {
                    sys_print("\b \b");
                    idx--;
                }
                history_view_idx--;
                strcpy_safe(cmd_buffer, history[history_view_idx]);
                idx = strlen(cmd_buffer);
                sys_print(cmd_buffer);
            }
        } else if (c == 0x12) {
            if (history_count > 0 && history_view_idx < history_count) {
                while (idx > 0) {
                    sys_print("\b \b");
                    idx--;
                }
                history_view_idx++;
                if (history_view_idx == history_count) {
                    cmd_buffer[0] = 0;
                    idx           = 0;
                } else {
                    strcpy_safe(cmd_buffer, history[history_view_idx]);
                    idx = strlen(cmd_buffer);
                    sys_print(cmd_buffer);
                }
            }
        } else if (c == 3) {
            sys_print("^C\n");
            idx           = 0;
            cmd_buffer[0] = '\0';
            print_prompt();
            continue;
        } else if (c == '\t') {
            if (idx > 0) {
                cmd_buffer[idx] = '\0';
                char entry_name[64];
                int  count = 0;
                while (sys_readdir(count, entry_name) != 0) {
                    int match = 1;
                    for (int i = 0; i < idx; i++) {
                        if (entry_name[i] != cmd_buffer[i]) {
                            match = 0;
                            break;
                        }
                    }
                    if (match) {
                        int p = idx;
                        while (entry_name[p] != '\0' && idx < CMD_LEN - 1) {
                            cmd_buffer[idx++] = entry_name[p];
                            char temp[2]      = {entry_name[p], '\0'};
                            sys_print(temp);
                            p++;
                        }
                        break;
                    }
                    count++;
                }
            }
        } else if (c == '\n') {
            sys_print("\n");
            cmd_buffer[idx] = '\0';
            if (idx > 0) {
                if (history_count < HISTORY_MAX) {
                    strcpy_safe(history[history_count], cmd_buffer);
                    history_count++;
                } else {
                    for (int i = 0; i < HISTORY_MAX - 1; i++)
                        strcpy_safe(history[i], history[i + 1]);
                    strcpy_safe(history[HISTORY_MAX - 1], cmd_buffer);
                }
                history_view_idx = history_count;
                execute_command(cmd_buffer);
            } else {
                execute_command("");
            }
            idx = 0;
            print_prompt();
        } else if (c == '\b') {
            if (idx > 0) {
                idx--;
                sys_print("\b \b");
            }
        } else if (c >= ' ' && c <= '~') {
            if (idx < CMD_LEN - 1) {
                cmd_buffer[idx++] = c;
                char temp[2]      = {c, '\0'};
                sys_print(temp);
            }
        }
    }
}
/*
 * A background task that runs indefinitely to verify the scheduler is functioning.
 * It increments a counter and outputs a heartbeat message periodically.
 */
void system_monitor_task() {
    int counter = 0;
    while (1) {
        counter++;
        if (counter % 500000 == 0) {
            serial_log(" [BG-TASK] System Alive. Tick...\n");
            counter = 0;
            sys_yield();
        }
    }
}
/*
 * Updates the window manager state and redraws the screen.
 * This task continuously processes mouse inputs and refreshes the display buffers,
 * yielding the CPU to other processes when idle.
 */
void compositor_task() {
    while (1) {
        wm_mouse_event(mouse_x, mouse_y, mouse_left_button);
        wm_refresh();
        sys_yield();
    }
}
/*
 * CORE SYSTEM INTEGRATION & SYNCHRONIZATION
 * 
 * kmain acts as the central integration point for all independent phases of the OS.
 * The boot sequence synchronizes the subsystems in a strict dependency order:
 * 1. CPU & Memory (Phase 2): GDT, IDT, PMM, and VMM establish the hardware isolation, 
 *    interrupt vectoring, and paged memory model.
 * 2. Hardware Drivers (Phase 3): The ATA disk, graphics framebuffer, and PS/2 mouse 
 *    are initialized, binding to the hardware states provided by the Multiboot handoff.
 * 3. File System (Phase 1): The VFS is mounted on top of the ATA driver, creating the 
 *    abstraction layer required by userland binaries.
 * 4. Graphics & GUI (Phase 4): The window manager is brought online, mapping the physical 
 *    framebuffer to the compositor's backbuffers.
 * 5. Process Scheduling (Phase 1 & 5): The multitasking engine is started, spawning the 
 *    core Ring 0 tasks (monitor, shell, compositor) that will eventually bootstrap the 
 *    Ring 3 userland processes (Phase 5).
 * 
 * This explicit ordering ensures that hardware dependencies are resolved before higher-level 
 * abstractions attempt to allocate memory or dispatch hardware interrupts.
 *
 * kmain is the kernel entry point called by the bootloader.
 * It initializes hardware subsystems, memory management, file systems, and graphics.
 * After setting up the window manager and spawning the initial tasks (monitor, shell, compositor),
 * it enables interrupts and enters a continuous halt loop to save power when idle.
 */
void kmain(multiboot_info_t *mboot_ptr, uint32_t magic) {
    init_serial();
    init_gdt();
    init_idt();
    init_pmm(mboot_ptr->mem_upper);
    init_vmm();
    init_heap();
    init_ata();
    init_fs(mboot_ptr);
    init_graphics(mboot_ptr);
    init_mouse();
    init_wm();
    console_win             = create_window("Terminal", 50, 50, 600, 400);
    console_win->text_color = 0xFFFFFFFF;
    console_win->bg_color   = 0xFF000000;
    for (int i = 0; i < 600 * 400; i++)
        console_win->buffer[i] = 0xFF000000;
    term_print(" [SYSTEM] Graphics Terminal Initialized.\n");
    init_multitasking();
    create_process(system_monitor_task, 0, 0, 1, 0);
    create_process(shell_task, 0, 0, 1, 0);
    create_process(compositor_task, 0, 0, 1, 0);
    __asm__ volatile("sti");
    while (1) {
        wm_mouse_event(mouse_x, mouse_y, mouse_left_button);
        wm_refresh();
        __asm__ volatile("hlt");
    }
}