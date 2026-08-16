#include "stdlib.h"

/*
 * sysmon program
 * 
 * A graphical system monitor that displays real-time statistics like memory
 * usage and process count. It polls the kernel for updates and renders a GUI.
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
    
    // Extract digits in reverse order.
    while (n > 0) {
        buffer[i++] = (n % 10) + '0';
        n /= 10;
    }
    if (is_neg)
        buffer[i++] = '-';
    buffer[i] = 0;
    
    // Reverse the string in place to obtain the correct order.
    for (int j = 0; j < i / 2; j++) {
        char t            = buffer[j];
        buffer[j]         = buffer[i - 1 - j];
        buffer[i - 1 - j] = t;
    }
}

// Entry point for the system monitor application.
int main() {
    sysinfo_t info;
    char      num_buf[16];
    
    // Request a new window from the window manager.
    int my_win = create_window("Glass Box", 680, 50, 300, 260);
    win_set_text_color(my_win, 0xFFFFFFFF);
    
    rect_t bg = {my_win, 0, 0, 300, 260, 0xFF1E1E1E};
    draw_rect(&bg);
    
    // Main loop.
    while (1) {
        // Query the kernel for the latest system state.
        get_sysinfo(&info);
        
        // Clear the text area from the previous frame.
        rect_t text_clear = {my_win, 10, 30, 280, 220, 0xFF1E1E1E};
        draw_rect(&text_clear);
        
        // Calculate memory usage percentage to draw the bar graph.
        int mem_percent = 0;
        if (info.total_memory_kb > 0) {
            mem_percent = (info.used_memory_kb * 100) / info.total_memory_kb;
        }
        int bar_width = (mem_percent * 280) / 100;
        
        // Draw the memory usage bar background.
        rect_t bar_bg = {my_win, 10, 60, 280, 20, 0xFF333333};
        draw_rect(&bar_bg);
        
        // Draw the filled portion of the memory usage bar.
        rect_t bar_fill = {my_win, 10, 60, bar_width, 20, 0xFF00FF00};
        draw_rect(&bar_fill);
        
        // Output system statistics text.
        win_set_cursor(my_win, 10, 10);
        win_print(my_win, "--- THE GLASS BOX ---");
        
        win_set_cursor(my_win, 10, 100);
        win_print(my_win, "Uptime Ticks : ");
        itoa(info.uptime_ticks, num_buf);
        win_print(my_win, num_buf);
        
        win_set_cursor(my_win, 10, 120);
        win_print(my_win, "Used Mem (KB): ");
        itoa(info.used_memory_kb, num_buf);
        win_print(my_win, num_buf);
        
        win_set_cursor(my_win, 10, 140);
        win_print(my_win, "Active Tasks : ");
        itoa(info.process_count, num_buf);
        win_print(my_win, num_buf);
        
        win_set_cursor(my_win, 10, 230);
        win_print(my_win, "Click titlebar to drag!");
        
        // Yield execution to allow other processes to run before updating again.
        for (int i = 0; i < 20; i++) {
            yield();
        }
    }
    return 0;
}
