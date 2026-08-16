#include <stdint.h>
#include "../drivers/vga.h"
#include "../kernel/process.h"
#include <stdbool.h>
extern uint8_t inb(uint16_t port);
extern void    outb(uint16_t port, uint8_t val);
extern void    kill_foreground_process();
extern void    wm_push_key(char c);
bool           shift_pressed = false;
bool           caps_lock     = false;
bool           ctrl_pressed  = false;
/*
 * Circular buffer for storing pending keyboard input.
 * Interrupt handlers place characters here, and user-space processes 
 * read from it. This isolates the timing of hardware interrupts from 
 * the timing of application reads.
 */
#define KBD_BUFFER_SIZE 256
char         kbd_buffer[KBD_BUFFER_SIZE];
volatile int read_ptr  = 0;
volatile int write_ptr = 0;

/*
 * Appends a character to the keyboard buffer.
 * If the buffer is full (next write pointer equals the current read pointer),
 * the character is dropped to prevent overwriting unread data.
 */
void         kbd_buffer_write(char c) {
    int next_write = (write_ptr + 1) % KBD_BUFFER_SIZE;
    if (next_write != read_ptr) {
        kbd_buffer[write_ptr] = c;
        write_ptr             = next_write;
    }
}

/*
 * Retrieves the next available character from the keyboard buffer.
 * Returns 0 if the buffer is empty. Updates the read pointer to the 
 * next position in the ring.
 */
char kbd_buffer_read() {
    if (read_ptr == write_ptr)
        return 0;
    char c   = kbd_buffer[read_ptr];
    read_ptr = (read_ptr + 1) % KBD_BUFFER_SIZE;
    return c;
}
char kbd_US_low[128]  = {0,   27,  '1', '2',  '3',         '4',        '5',  '6',  '7', '8', '9',
                         '0', '-', '=', '\b', '\t',        'q',        'w',  'e',  'r', 't', 'y',
                         'u', 'i', 'o', 'p',  '[',         ']',        '\n', 0,    'a', 's', 'd',
                         'f', 'g', 'h', 'j',  'k',         'l',        ';',  '\'', '`', 0,   '\\',
                         'z', 'x', 'c', 'v',  'b',         'n',        'm',  ',',  '.', '/', 0,
                         '*', 0,   ' ', 0,    [72] = 0x11, [80] = 0x12};
char kbd_US_high[128] = {
    0,    27,  '!', '@', '#', '$', '%', '^', '&', '*', '(',  ')', '_', '+',  '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',  '{', '}', '\n', 0,
    'A',  'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 0,   '|',  'Z',
    'X',  'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,   '*',  0,   ' ', 0,
};

/*
 * Interrupt handler for PS/2 keyboard events on IRQ1.
 * Reads the scancode from I/O port 0x60 and sends an End of Interrupt (EOI) 
 * to the PIC (0x20). It tracks the state of modifier keys (Shift, Caps Lock, Ctrl) 
 * by checking for key press and release (bit 7 set) events. Decoded characters 
 * are pushed to the window manager and unblock any waiting processes.
 */
void keyboard_handler() {
    uint8_t scancode = inb(0x60);
    outb(0x20, 0x20);
    if (scancode == 0xE0)
        return;
    if (scancode & 0x80) {
        uint8_t released = scancode & 0x7F;
        if (released == 42 || released == 54)
            shift_pressed = false;
        if (released == 29)
            ctrl_pressed = false;
        return;
    }
    if (scancode == 42 || scancode == 54) {
        shift_pressed = true;
        return;
    }
    if (scancode == 58) {
        caps_lock = !caps_lock;
        return;
    }
    if (scancode == 29) {
        ctrl_pressed = true;
        return;
    }
    if (scancode < 128) {
        char c = 0;
        if (ctrl_pressed && scancode == 46) {
            kill_foreground_process();
            return;
        }
        if (shift_pressed) {
            c = kbd_US_high[scancode];
        } else {
            c = kbd_US_low[scancode];
            if (caps_lock && c >= 'a' && c <= 'z')
                c -= 32;
        }
        if (c != 0) {
            wm_push_key(c);
            process_unblock(1);
        }
    }
}