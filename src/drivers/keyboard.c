#include <stdint.h>
#include "../drivers/vga.h"
#include "../kernel/process.h"
#include <stdbool.h>

extern uint8_t inb(uint16_t port);
bool shift_pressed = false;
bool caps_lock = false;

// --- Circular Buffer ---
#define KBD_BUFFER_SIZE 256
char kbd_buffer[KBD_BUFFER_SIZE];
volatile int read_ptr = 0;
volatile int write_ptr = 0;

void kbd_buffer_write(char c) {
    int next_write = (write_ptr + 1) % KBD_BUFFER_SIZE;
    if (next_write != read_ptr) { 
        kbd_buffer[write_ptr] = c;
        write_ptr = next_write;
    }
}

char kbd_buffer_read() {
    if (read_ptr == write_ptr) return 0;
    char c = kbd_buffer[read_ptr];
    read_ptr = (read_ptr + 1) % KBD_BUFFER_SIZE;
    return c;
}

// Lowercase / Default Map
char kbd_US_low[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,
  '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,
  '*', 0, ' ', 0,
  // Maps Arrow Keys to special codes
  [72] = 0x11, [80] = 0x12
};

// Shifted Map (Symbols & Capitals)
char kbd_US_high[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
  '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~',   0,
  '|',  'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',   0,
  '*', 0, ' ', 0,
};

void keyboard_handler() {
    uint8_t scancode = inb(0x60);
    
    // Ignore prefix bytes
    if (scancode == 0xE0) return;

    // KEY RELEASED (Top bit set)
    if (scancode & 0x80) {
        uint8_t released = scancode & 0x7F;
        if (released == 42 || released == 54) { // Left/Right Shift
            shift_pressed = false;
        }
        return;
    }

    // KEY PRESSED
    // Check Shifters
    if (scancode == 42 || scancode == 54) {
        shift_pressed = true;
        return;
    }
    if (scancode == 58) { // Caps Lock
        caps_lock = !caps_lock;
        return;
    }

    if (scancode < 128) {
        char c = 0;
        
        // Logic: Shift XOR CapsLock (for letters)
        if (shift_pressed) {
            c = kbd_US_high[scancode];
        } else {
            c = kbd_US_low[scancode];
            // Apply Caps Lock just to letters (a-z)
            if (caps_lock && c >= 'a' && c <= 'z') {
                c -= 32; // Convert to upper
            }
        }

        if (c != 0) {
            kbd_buffer_write(c);
            process_unblock(1);
        }
    }
}