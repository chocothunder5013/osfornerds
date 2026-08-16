#include "stdlib.h"

int  my_win;

/*
 * stress program
 * 
 * Provides diagnostic tools to stress the CPU or memory allocator.
 * This helps test multitasking and system stability under load.
 */

// Convert an integer to a null-terminated string representation.
void itoa(int n, char *buffer) {
    if (n == 0) {
        buffer[0] = '0';
        buffer[1] = 0;
        return;
    }
    int i = 0, is_neg = 0;
    if (n < 0) {
        is_neg = 1;
        n      = -n;
    }
    while (n > 0) {
        buffer[i++] = (n % 10) + '0';
        n /= 10;
    }
    if (is_neg)
        buffer[i++] = '-';
    buffer[i] = 0;
    
    for (int j = 0; j < i / 2; j++) {
        char t            = buffer[j];
        buffer[j]         = buffer[i - 1 - j];
        buffer[i - 1 - j] = t;
    }
}

// Perform a CPU-intensive task by calculating prime numbers indefinitely.
void cpu_stress() {
    win_print(my_win, "
[STRESS] CPU Burner Started...
");
    win_print(my_win, "[STRESS] Check Active Tasks in Sysmon!
");
    int  n = 2;
    char num_buf[16];
    
    while (1) {
        int is_prime = 1;
        for (int i = 2; i * i <= n; i++) {
            if (n % i == 0) {
                is_prime = 0;
                break;
            }
        }
        if (n % 10000 == 0) {
            win_print(my_win, "Calculated up to: ");
            itoa(n, num_buf);
            win_print(my_win, num_buf);
            win_print(my_win, "
");
        }
        n++;
    }
}

// Perform a memory-intensive task by continuously allocating memory.
void mem_stress() {
    win_print(my_win, "
[STRESS] Memory Hog Started...
");
    win_print(my_win, "[STRESS] Watch the green bar fill up!
");
    int  total_alloc = 0;
    char num_buf[16];
    
    while (1) {
        // Allocate a page of memory.
        char *leak = (char *)malloc(4096);
        if (!leak) {
            win_set_text_color(my_win, 0xFFFF0000);
            win_print(my_win, "
[STRESS] OOM KILLER INCOMING!
");
            // Halt allocation and yield indefinitely.
            while (1)
                yield();
        }
        
        // Touch the memory to ensure physical pages are mapped.
        for (int i = 0; i < 4096; i += 1024)
            leak[i] = 'X';
            
        total_alloc += 4;
        
        if (total_alloc % 1024 == 0) {
            win_print(my_win, "Leaked MB: ");
            itoa(total_alloc / 1024, num_buf);
            win_print(my_win, num_buf);
            win_print(my_win, "
");
        }
        
        // Yield occasionally to allow the rest of the system to remain responsive.
        for (int i = 0; i < 5; i++)
            yield();
    }
}

// Entry point for the diagnostic suite.
int main() {
    my_win    = create_window("Chaos Suite", 100, 100, 350, 250);
    rect_t bg = {my_win, 0, 0, 350, 250, 0xFF222222};
    draw_rect(&bg);
    win_set_cursor(my_win, 10, 10);
    win_set_text_color(my_win, 0xFFFFFFFF);
    
    win_print(my_win, "--- DIAGNOSTICS SUITE ---

");
    win_print(my_win, "1. CPU Burner
");
    win_print(my_win, "2. Memory Hog

");
    win_print(my_win, "Select a test (1-2): ");
    
    char choice = 0;
    while (choice != '1' && choice != '2') {
        choice = get_char();
    }
    
    char temp[2] = {choice, 0};
    win_print(my_win, temp);
    win_print(my_win, "
");
    
    if (choice == '1')
        cpu_stress();
    else if (choice == '2')
        mem_stress();
        
    return 0;
}
