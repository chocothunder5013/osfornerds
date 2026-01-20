/* src/drivers/mouse.c */
#include <stdint.h>
#include "serial.h" 
#include "../cpu/idt.h" 

// Externs from graphics.c
extern int screen_w;
extern int screen_h;

// Mouse State (Global)
int mouse_x = 0; 
int mouse_y = 0;
int mouse_left_button = 0;
int mouse_right_button = 0;

static uint8_t mouse_cycle = 0;     
static int8_t  mouse_byte[3];

void mouse_wait(uint8_t type) {
    uint32_t time_out = 100000;
    if (type == 0) {
        while (time_out--) { if ((inb(0x64) & 1) == 1) return; }
    } else {
        while (time_out--) { if ((inb(0x64) & 2) == 0) return; }
    }
}

void mouse_write(uint8_t write) {
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, write);
}

uint8_t mouse_read() {
    mouse_wait(0);
    return inb(0x60);
}

// The Interrupt Handler
void mouse_handler() {
    uint8_t status = inb(0x64);
    if (!(status & 0x20)) return;

    uint8_t val = inb(0x60);
    mouse_byte[mouse_cycle++] = val;

    if (mouse_cycle == 3) { 
        mouse_cycle = 0;

        // Packet Format:
        // Byte 0: Y_Over X_Over Y_Sign X_Sign 1 Middle Right Left
        uint8_t flags = mouse_byte[0];
        int8_t x_rel = mouse_byte[1];
        int8_t y_rel = mouse_byte[2];

        // Button State
        mouse_left_button = (flags & 0x01);
        mouse_right_button = (flags & 0x02);

        // Movement (Handle overflow bits if you want perfection, skipping for simplicity)
        // Note: Some PS/2 mice send 9-bit values. This is "good enough" for now.
        
        mouse_x += x_rel;
        mouse_y -= y_rel; // Y is inverted in PS/2

        // Clamp to Screen Dimensions
        if (mouse_x < 0) mouse_x = 0;
        if (mouse_x >= screen_w - 1) mouse_x = screen_w - 1;
        
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_y >= screen_h - 1) mouse_y = screen_h - 1;

        // CRITICAL: Do NOT draw here. The compositor (wm.c) will see the new x/y 
        // and draw it on the next frame refresh.
    }
}

void init_mouse() {
    // Defaults if screen is not initialized yet
    if (screen_w == 0) { screen_w = 1024; screen_h = 768; }
    mouse_x = screen_w / 2;
    mouse_y = screen_h / 2;

    mouse_wait(1);
    outb(0x64, 0xA8); // Enable Aux
    mouse_wait(1);
    outb(0x64, 0x20); // Get Status
    uint8_t status = mouse_read();
    status |= 2;      // Enable IRQ12
    status &= ~0x20;  // Disable Clock
    mouse_wait(1);
    outb(0x64, 0x60); // Set Status
    mouse_wait(1);
    outb(0x60, status);

    mouse_write(0xF6); // Reset
    mouse_read();
    mouse_write(0xF4); // Enable Streaming
    mouse_read();
}