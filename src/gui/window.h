#ifndef WINDOW_H
#define WINDOW_H

#include <stdint.h>

typedef struct window {
    int x, y;
    int width, height;
    char title[32];
    uint32_t* buffer;
    struct window* next;
    
    // Dragging state
    int is_dragging;
    int drag_offset_x;
    int drag_offset_y;

    // --- NEW: Terminal State ---
    int cursor_x;
    int cursor_y;
    uint32_t text_color;
    uint32_t bg_color;
} window_t;

void init_wm();
window_t* create_window(const char* title, int x, int y, int w, int h);
void wm_refresh(); // The "Compositor" function
void wm_mouse_event(int x, int y, int left_click);

#endif