/*
 * Window Manager API
 *
 * Defines the core structures and functions for the window management system.
 * The system relies on off-screen buffering for individual windows, which are
 * then composited onto a global backbuffer to prevent tearing before being
 * pushed to the display.
 */
#ifndef WINDOW_H
#define WINDOW_H
#include <stdint.h>

/*
 * Represents a single GUI window.
 *
 * Each window maintains its own local pixel buffer, isolating its rendering 
 * from the rest of the screen. The window manager iterates through a linked 
 * list of these structures to composite the final image.
 *
 * The order of windows in the linked list determines their z-order during 
 * compositing. Windows rendered later appear on top of earlier ones.
 */
typedef struct window {
    int            x, y;
    int            width, height;
    char           title[32];
    uint32_t      *buffer;
    struct window *next;
    int            owner_pid;
    char           kbd_buffer[256];
    int            kbd_read_ptr;
    int            kbd_write_ptr;
    int            cursor_x;
    int            cursor_y;
    uint32_t       text_color;
    uint32_t       bg_color;
} window_t;

void      init_wm();
window_t *create_window(const char *title, int x, int y, int w, int h);
void      wm_refresh();
void      wm_mouse_event(int x, int y, int left_click);
void      wm_push_key(char c);
char      wm_pop_key(int pid);
#endif