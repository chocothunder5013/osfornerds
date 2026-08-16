/*
 * Window Manager
 *
 * Manages windows, routes input events to the active window,
 * and composites the display into a backbuffer.
 *
 * This implementation uses a double-buffering approach. Individual windows 
 * draw to their own allocated memory buffers. The window manager composites 
 * these local buffers into a single global backbuffer. Once compositing is 
 * complete, the entire backbuffer is copied to the hardware framebuffer. 
 * This approach eliminates screen tearing and flickering that would occur 
 * if windows were drawn directly to the screen.
 */
#include "window.h"
#include "../mm/heap.h"
#include "../drivers/font.h"
extern int       screen_w;
extern int       screen_h;
extern uint32_t *framebuffer;
extern void      draw_char(int x, int y, char c, uint32_t color);
extern void      put_pixel(int x, int y, uint32_t color);
typedef struct process {
    int pid;
} process_t;
extern process_t *current_process;
uint32_t         *backbuffer    = 0;
window_t         *head          = 0;
window_t         *tail          = 0;
window_t         *active_window = 0;
int               window_count  = 0;
/* 
 * A memory pool for window buffers. 
 * Pre-allocating a fixed pool avoids the overhead and fragmentation 
 * of dynamic heap allocations for large pixel arrays. 
 */
#define MAX_WINDOWS 8
static uint32_t *window_memory_pool              = 0;
static int       window_memory_used[MAX_WINDOWS] = {0};
uint32_t        *allocate_window_buffer() {
    if (screen_w == 0)
        return 0;
    if (!window_memory_pool) {
        window_memory_pool = (uint32_t *)kmalloc(MAX_WINDOWS * screen_w * screen_h * 4);
        if (!window_memory_pool)
            return 0;
    }
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (!window_memory_used[i]) {
            window_memory_used[i] = 1;
            return &window_memory_pool[i * (screen_w * screen_h)];
        }
    }
    return 0;
}
void free_window_buffer(uint32_t *ptr) {
    if (!window_memory_pool)
        return;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (&window_memory_pool[i * (screen_w * screen_h)] == ptr) {
            window_memory_used[i] = 0;
            return;
        }
    }
}
static const uint8_t cursor_bitmap[16][16] = {{1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                              {1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                              {1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                              {1, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                              {1, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                              {1, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                              {1, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0},
                                              {1, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0},
                                              {1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0},
                                              {1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0},
                                              {1, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0},
                                              {1, 2, 2, 1, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0},
                                              {1, 2, 1, 0, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0},
                                              {1, 1, 0, 0, 0, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0},
                                              {1, 0, 0, 0, 0, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0},
                                              {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0}};
void                 init_wm() {
    if (screen_w == 0 || screen_h == 0)
        return;
    backbuffer = (uint32_t *)kmalloc(screen_w * screen_h * 4);
    for (int i = 0; i < screen_w * screen_h; i++) {
        backbuffer[i] = 0xFF1E2127;
    }
}
void wm_resize_window(window_t *win, int new_w, int new_h) {
    if (win->width == new_w && win->height == new_h)
        return;
    uint32_t *new_buf = allocate_window_buffer();
    if (!new_buf)
        return;
    for (int i = 0; i < new_w * new_h; i++) {
        new_buf[i] = win->bg_color;
    }
    if (win->buffer) {
        int min_w = (win->width < new_w) ? win->width : new_w;
        int min_h = (win->height < new_h) ? win->height : new_h;
        for (int j = 0; j < min_h; j++) {
            for (int i = 0; i < min_w; i++) {
                new_buf[j * new_w + i] = win->buffer[j * win->width + i];
            }
        }
        free_window_buffer(win->buffer);
    }
    win->buffer = new_buf;
    win->width  = new_w;
    win->height = new_h;
}
/*
 * Adjusts window positions and sizes for a simple tiling layout.
 *
 * This rearranges all open windows to fill the screen without overlapping. 
 * The first window is given the left side of the screen, and subsequent 
 * windows are stacked vertically on the right side.
 */
void recalculate_tiling() {
    if (!head)
        return;
    window_t *curr = head;
    if (window_count == 1) {
        curr->x = 0;
        curr->y = 20;
        wm_resize_window(curr, screen_w, screen_h - 20);
    } else {
        curr->x = 0;
        curr->y = 20;
        wm_resize_window(curr, screen_w / 2, screen_h - 20);
        curr            = curr->next;
        int stack_count = window_count - 1;
        int stack_h     = (screen_h - 20) / stack_count;
        int current_y   = 20;
        while (curr) {
            curr->x = screen_w / 2;
            curr->y = current_y;
            wm_resize_window(curr, screen_w / 2, stack_h);
            current_y += stack_h;
            curr = curr->next;
        }
    }
}
window_t *create_window(const char *title, int x, int y, int w, int h) {
    window_t *win = (window_t *)kmalloc(sizeof(window_t));
    int       i   = 0;
    while (title[i] && i < 31) {
        win->title[i] = title[i];
        i++;
    }
    win->title[i]      = 0;
    win->owner_pid     = current_process ? current_process->pid : 0;
    win->kbd_read_ptr  = 0;
    win->kbd_write_ptr = 0;
    win->cursor_x      = 0;
    win->cursor_y      = 0;
    win->text_color    = 0xFFFFFFFF;
    win->bg_color      = 0xFF000000;
    win->width         = w;
    win->height        = h;
    win->buffer        = allocate_window_buffer();
    if (win->buffer) {
        for (int j = 0; j < w * h; j++)
            win->buffer[j] = win->bg_color;
    }
    win->next = 0;
    if (!head) {
        head = win;
        tail = win;
    } else {
        tail->next = win;
        tail       = win;
    }
    window_count++;
    if (!active_window)
        active_window = win;
    recalculate_tiling();
    return win;
}
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
void wm_draw_char_to_backbuffer(int x, int y, char c, uint32_t color) {
    const uint8_t *glyph = font8x8_basic[(int)c];
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (glyph[row] & (1 << col)) {
                int bx = x + (7 - col);
                int by = y + row;
                if (bx >= 0 && bx < screen_w && by >= 0 && by < screen_h) {
                    backbuffer[by * screen_w + bx] = color;
                }
            }
        }
    }
}
void wm_draw_string_to_backbuffer(int x, int y, const char *str, uint32_t color) {
    while (*str) {
        wm_draw_char_to_backbuffer(x, y, *str++, color);
        x += 8;
    }
}
/*
 * Composites the entire screen and updates the display.
 *
 * This function handles z-ordering through a painter's algorithm. 
 * It iterates through the linked list of windows, starting from the head. 
 * Because each window is drawn over the previous ones, the order of the list 
 * implicitly defines the z-order (head is at the bottom, tail is at the top).
 *
 * 1. The backbuffer is cleared.
 * 2. Windows are drawn back-to-front, including their title bars.
 * 3. The mouse cursor is drawn last so it always appears on top.
 * 4. The finished backbuffer is blitted to the hardware framebuffer.
 */
void wm_refresh() {
    if (!backbuffer)
        return;
    for (int i = 0; i < screen_w * screen_h; i++)
        backbuffer[i] = 0xFF1E2127;
    window_t *curr = head;
    while (curr) {
        uint32_t title_color = (curr == active_window) ? 0xFF0078D7 : 0xFF333333;
        wm_draw_rect_to_backbuffer(curr->x, curr->y - 20, curr->width, 20, title_color);
        char title_buf[64];
        int  idx = 0;
        while (curr->title[idx] && idx < 31) {
            title_buf[idx] = curr->title[idx];
            idx++;
        }
        title_buf[idx++] = ' ';
        title_buf[idx++] = '[';
        title_buf[idx++] = 'P';
        title_buf[idx++] = 'I';
        title_buf[idx++] = 'D';
        title_buf[idx++] = ':';
        title_buf[idx++] = ' ';
        int n            = curr->owner_pid;
        if (n == 0) {
            title_buf[idx++] = '0';
        } else {
            char num[10];
            int  num_idx = 0;
            while (n > 0) {
                num[num_idx++] = (n % 10) + '0';
                n /= 10;
            }
            for (int i = num_idx - 1; i >= 0; i--) {
                title_buf[idx++] = num[i];
            }
        }
        title_buf[idx++] = ']';
        title_buf[idx]   = 0;
        wm_draw_string_to_backbuffer(curr->x + 5, curr->y - 14, title_buf, 0xFFFFFFFF);
        wm_draw_rect_to_backbuffer(curr->x + curr->width - 20, curr->y - 20, 20, 20, 0xFFD32F2F);
        wm_draw_char_to_backbuffer(curr->x + curr->width - 14, curr->y - 14, 'X', 0xFFFFFFFF);
        for (int j = 0; j < curr->height; j++) {
            for (int i = 0; i < curr->width; i++) {
                int screen_x = curr->x + i;
                int screen_y = curr->y + j;
                if (screen_x >= 0 && screen_x < screen_w && screen_y >= 0 && screen_y < screen_h) {
                    uint32_t color                             = curr->buffer[j * curr->width + i];
                    backbuffer[screen_y * screen_w + screen_x] = color;
                }
            }
        }
        curr = curr->next;
    }
    extern int mouse_x, mouse_y;
    int        mx = mouse_x;
    int        my = mouse_y;
    for (int j = 0; j < 16; j++) {
        for (int i = 0; i < 16; i++) {
            uint8_t pixel = cursor_bitmap[j][i];
            if (pixel == 0)
                continue;
            int sx = mx + i;
            int sy = my + j;
            if (sx >= 0 && sx < screen_w && sy >= 0 && sy < screen_h) {
                backbuffer[sy * screen_w + sx] = (pixel == 1) ? 0xFF000000 : 0xFFFFFFFF;
            }
        }
    }
    uint32_t  total_pixels = screen_w * screen_h;
    uint32_t *dest         = framebuffer;
    uint32_t *src          = backbuffer;
    for (uint32_t i = 0; i < total_pixels / 8; i++) {
        *dest++ = *src++;
        *dest++ = *src++;
        *dest++ = *src++;
        *dest++ = *src++;
        *dest++ = *src++;
        *dest++ = *src++;
        *dest++ = *src++;
        *dest++ = *src++;
    }
}
/*
 * Processes mouse input and handles hit testing.
 *
 * When a left click occurs, this function checks the coordinates against 
 * the bounding boxes of all windows. It iterates through the window list 
 * to find if the user clicked inside a window or on its close button.
 */
void wm_mouse_event(int x, int y, int left_click) {
    static int prev_left_click = 0;
    if (left_click && !prev_left_click) {
        window_t *curr = head;
        while (curr) {
            if (x >= curr->x + curr->width - 20 && x < curr->x + curr->width && y >= curr->y - 20 &&
                y < curr->y) {
                extern int kill_process(int pid);
                kill_process(curr->owner_pid);
                break;
            }
            if (x >= curr->x && x < curr->x + curr->width && y >= curr->y - 20 &&
                y < curr->y + curr->height) {
                active_window = curr;
                break;
            }
            curr = curr->next;
        }
    }
    prev_left_click = left_click;
}
void wm_scroll_window(window_t *win) {
    int total_rows    = win->height;
    int scroll_height = 8;
    for (int i = 0; i < (total_rows - scroll_height) * win->width; i++) {
        win->buffer[i] = win->buffer[i + (scroll_height * win->width)];
    }
    int start_index = (total_rows - scroll_height) * win->width;
    for (int i = start_index; i < total_rows * win->width; i++) {
        win->buffer[i] = win->bg_color;
    }
    win->cursor_y -= 8;
    if (win->cursor_y < 0)
        win->cursor_y = 0;
}
void wm_putc(window_t *win, char c) {
    if (!win)
        return;
    if (c == '\n') {
        win->cursor_x = 0;
        win->cursor_y += 8;
    } else if (c == '\b') {
        if (win->cursor_x >= 8) {
            win->cursor_x -= 8;
            for (int dy = 0; dy < 8; dy++) {
                for (int dx = 0; dx < 8; dx++) {
                    int offset          = (win->cursor_y + dy) * win->width + (win->cursor_x + dx);
                    win->buffer[offset] = win->bg_color;
                }
            }
        }
    } else {
        if (win->cursor_x + 8 >= win->width) {
            win->cursor_x = 0;
            win->cursor_y += 8;
        }
        const uint8_t *glyph = font8x8_basic[(int)c];
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                if (glyph[row] & (1 << col)) {
                    int offset = (win->cursor_y + row) * win->width + (win->cursor_x + (7 - col));
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
void wm_print(window_t *win, const char *str) {
    while (*str) {
        wm_putc(win, *str++);
    }
}
void wm_push_key(char c) {
    if (!active_window)
        return;
    int next = (active_window->kbd_write_ptr + 1) % 256;
    if (next != active_window->kbd_read_ptr) {
        active_window->kbd_buffer[active_window->kbd_write_ptr] = c;
        active_window->kbd_write_ptr                            = next;
    }
}
char wm_pop_key(int pid) {
    window_t *curr = head;
    while (curr) {
        if (curr->owner_pid == pid || curr->owner_pid == 0) {
            if (curr->kbd_read_ptr == curr->kbd_write_ptr) {
                curr = curr->next;
                continue;
            }
            char c             = curr->kbd_buffer[curr->kbd_read_ptr];
            curr->kbd_read_ptr = (curr->kbd_read_ptr + 1) % 256;
            return c;
        }
        curr = curr->next;
    }
    return 0;
}
void wm_destroy_window(int pid) {
    if (pid <= 1)
        return;
    window_t *curr = head;
    window_t *prev = 0;
    while (curr) {
        if (curr->owner_pid == pid) {
            if (prev)
                prev->next = curr->next;
            else
                head = curr->next;
            if (curr == tail)
                tail = prev;
            if (curr == active_window)
                active_window = head;
            if (curr->buffer)
                free_window_buffer(curr->buffer);
            kfree(curr);
            window_count--;
            recalculate_tiling();
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}