/* src/gui/wm.c */
#include "window.h"
#include "../mm/heap.h"
#include "../drivers/font.h"

// Externs from graphics.c
extern int screen_w;
extern int screen_h;
extern uint32_t* framebuffer; // Video Memory
extern void draw_char(int x, int y, char c, uint32_t color);
extern void put_pixel(int x, int y, uint32_t color);

// The Backbuffer (Off-screen canvas)
uint32_t* backbuffer = 0;

// Window List
window_t* head = 0;
window_t* tail = 0; 

void init_wm() {
    // 1. Allocate Backbuffer
    if (screen_w == 0 || screen_h == 0) return;
    backbuffer = (uint32_t*)kmalloc(screen_w * screen_h * 4);
    
    // 2. Clear it to Blue
    for (int i = 0; i < screen_w * screen_h; i++) {
        backbuffer[i] = 0xFF0000AA; 
    }
}

// Create a new window
window_t* create_window(const char* title, int x, int y, int w, int h) {
    window_t* win = (window_t*)kmalloc(sizeof(window_t));
    
    // Copy Title
    int i = 0;
    while(title[i] && i < 31) { win->title[i] = title[i]; i++; }
    win->title[i] = 0;

    win->x = x; win->y = y;
    win->width = w; win->height = h;
    win->is_dragging = 0;
    win->cursor_x = 0;
    win->cursor_y = 0;
    win->text_color = 0xFF000000; 
    win->bg_color = 0xFFCCCCCC;   
    
    // Allocate Window Content Buffer
    win->buffer = (uint32_t*)kmalloc(w * h * 4);
    for(int j=0; j<w*h; j++) win->buffer[j] = 0xFFCCCCCC; 

    // Add to list
    win->next = 0;
    if (!head) {
        head = win;
        tail = win;
    } else {
        tail->next = win;
        tail = win;
    }
    return win;
}

// Helper: Copy a rectangle to the backbuffer
void wm_draw_rect_to_backbuffer(int x, int y, int w, int h, uint32_t color) {
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            int bx = x + i;
            int by = y + j;
            if (bx >= 0 && bx < screen_w && by >= 0 && by < screen_h) {
                backbuffer[by * screen_w + bx] = color;
            }
        }
    }
}

// The Compositor
void wm_refresh() {
    if (!backbuffer) return;

    // 1. Draw Desktop Background
    for (int i = 0; i < screen_w * screen_h; i++) backbuffer[i] = 0xFF0000AA;

    // 2. Draw Windows
    window_t* curr = head;
    while (curr) {
        // A. Draw Title Bar
        wm_draw_rect_to_backbuffer(curr->x, curr->y - 20, curr->width, 20, 0xFF000088);
        
        // B. Draw Content
        for (int j = 0; j < curr->height; j++) {
            for (int i = 0; i < curr->width; i++) {
                int screen_x = curr->x + i;
                int screen_y = curr->y + j;
                
                if (screen_x >= 0 && screen_x < screen_w && screen_y >= 0 && screen_y < screen_h) {
                    uint32_t color = curr->buffer[j * curr->width + i];
                    backbuffer[screen_y * screen_w + screen_x] = color;
                }
            }
        }
        curr = curr->next;
    }

    // 3. Draw Mouse
    extern int mouse_x, mouse_y;
    int mx = mouse_x; int my = mouse_y;
    for(int j=0; j<10; j++) {
        for(int i=0; i<10; i++) {
            if (i <= j) { 
                int sx = mx + i; int sy = my + j;
                if(sx < screen_w && sy < screen_h)
                    backbuffer[sy * screen_w + sx] = 0xFFFFFFFF;
            }
        }
    }

    // 4. FLIP
    for (int i = 0; i < screen_w * screen_h; i++) {
        framebuffer[i] = backbuffer[i];
    }
}

// THIS WAS MISSING: The function main.c calls!
void wm_mouse_event(int x, int y, int left_click) {
    static int prev_left_click = 0;
    
    // 1. Handle Dragging
    window_t* curr = head;
    while (curr) {
        if (curr->is_dragging) {
            if (left_click) {
                curr->x = x - curr->drag_offset_x;
                curr->y = y - curr->drag_offset_y;
            } else {
                curr->is_dragging = 0; 
            }
            return; // Don't click other windows while dragging
        }
        curr = curr->next;
    }

    // 2. Handle New Click (Hit Test)
    if (left_click && !prev_left_click) {
        curr = head;
        while(curr) {
            // Hit Title Bar? (y - 20 to y)
            if (x >= curr->x && x < curr->x + curr->width &&
                y >= curr->y - 20 && y < curr->y) {
                
                curr->is_dragging = 1;
                curr->drag_offset_x = x - curr->x;
                curr->drag_offset_y = y - curr->y;
                break;
            }
            curr = curr->next;
        }
    }
    prev_left_click = left_click;
}

// Helper: Scroll a specific window's buffer up
void wm_scroll_window(window_t* win) {
    int total_rows = win->height;
    int scroll_height = 8; 

    // 1. Move memory up
    for (int i = 0; i < (total_rows - scroll_height) * win->width; i++) {
        win->buffer[i] = win->buffer[i + (scroll_height * win->width)];
    }

    // 2. Clear bottom lines
    int start_index = (total_rows - scroll_height) * win->width;
    for (int i = start_index; i < total_rows * win->width; i++) {
        win->buffer[i] = win->bg_color;
    }

    win->cursor_y -= 8;
    if (win->cursor_y < 0) win->cursor_y = 0;
}

// Draw a character to a window
void wm_putc(window_t* win, char c) {
    if (!win) return;

    if (c == '\n') {
        win->cursor_x = 0;
        win->cursor_y += 8;
    } 
    else if (c == '\b') {
        if (win->cursor_x >= 8) {
            win->cursor_x -= 8;
            for(int dy=0; dy<8; dy++) {
                for(int dx=0; dx<8; dx++) {
                   int offset = (win->cursor_y + dy) * win->width + (win->cursor_x + dx);
                   win->buffer[offset] = win->bg_color;
                }
            }
        }
    }
    else {
        if (win->cursor_x + 8 >= win->width) {
            win->cursor_x = 0;
            win->cursor_y += 8;
        }

        const uint8_t* glyph = font8x8_basic[(int)c];
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                if (glyph[row] & (1 << col)) {
                    int offset = (win->cursor_y + row) * win->width + (win->cursor_x + (7-col));
                    if (offset < win->width * win->height) {
                        win->buffer[offset] = win->text_color;
                    }
                }
            }
        }
        win->cursor_x += 8;
    }

    if (win->cursor_y + 8 >= win->height) {
        wm_scroll_window(win);
    }
}

// Print string to window
void wm_print(window_t* win, const char* str) {
    while (*str) {
        wm_putc(win, *str++);
    }
}