#include <stdint.h>
#include "serial.h"
#include "../cpu/idt.h"
extern int     screen_w;
extern int     screen_h;
extern void    outb(uint16_t port, uint8_t val);
int            mouse_x            = 0;
int            mouse_y            = 0;
int            mouse_left_button  = 0;
int            mouse_right_button = 0;
static uint8_t mouse_cycle        = 0;
static int8_t  mouse_byte[3];

/*
 * Polls the PS/2 controller status register (0x64) until it is ready.
 * type == 0: Wait for data to be available to read (bit 0 set).
 * type == 1: Wait for the input buffer to be empty before writing (bit 1 clear).
 */
void           mouse_wait(uint8_t type) {
    uint32_t time_out = 100000;
    if (type == 0) {
        while (time_out--) {
            if ((inb(0x64) & 1) == 1)
                return;
        }
    } else {
        while (time_out--) {
            if ((inb(0x64) & 2) == 0)
                return;
        }
    }
}

/*
 * Sends a command byte to the mouse device.
 * First writes the command 0xD4 to the status port (0x64) to indicate the 
 * next byte is for the second PS/2 port (the mouse). Then writes the actual 
 * command byte to the data port (0x60).
 */
void mouse_write(uint8_t write) {
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, write);
}

/*
 * Reads a single data byte from the PS/2 controller's data port (0x60).
 * Waits until data is available before reading.
 */
uint8_t mouse_read() {
    mouse_wait(0);
    return inb(0x60);
}

/*
 * Interrupt handler for PS/2 mouse events on IRQ12.
 * The mouse sends data in 3-byte packets. We read the status port to verify 
 * data is from the mouse, then read the data port. Once all 3 bytes are collected,
 * we extract button states (left, right) and relative movement (X, Y), update 
 * the global mouse coordinates while enforcing screen bounds, and send an EOI 
 * to both the master and slave PICs.
 */
void mouse_handler() {
    uint8_t status = inb(0x64);
    if (!(status & 0x20))
        return;
    uint8_t val = inb(0x60);
    if (mouse_cycle == 0 && !(val & 0x08))
        return;
    mouse_byte[mouse_cycle++] = val;
    if (mouse_cycle == 3) {
        mouse_cycle        = 0;
        int8_t  x_rel      = mouse_byte[1];
        int8_t  y_rel      = mouse_byte[2];
        uint8_t flags      = mouse_byte[0];
        mouse_left_button  = (flags & 0x01);
        mouse_right_button = (flags & 0x02);
        mouse_x += x_rel;
        mouse_y -= y_rel;
        if (mouse_x < 0)
            mouse_x = 0;
        if (mouse_x >= screen_w - 1)
            mouse_x = screen_w - 1;
        if (mouse_y < 0)
            mouse_y = 0;
        if (mouse_y >= screen_h - 1)
            mouse_y = screen_h - 1;
        outb(0xA0, 0x20);
        outb(0x20, 0x20);
    }
}

/*
 * Initializes the PS/2 mouse interface.
 * Enables the auxiliary device (mouse port) and configures the PS/2 controller 
 * to generate IRQ12. The mouse is reset, its default settings are applied, 
 * and packet streaming is enabled.
 */
void init_mouse() {
    if (screen_w == 0) {
        screen_w = 1024;
        screen_h = 768;
    }
    mouse_x = screen_w / 2;
    mouse_y = screen_h / 2;
    mouse_wait(1);
    outb(0x64, 0xA8);
    mouse_wait(1);
    outb(0x64, 0x20);
    uint8_t status = mouse_read();
    status |= 2;
    status &= ~0x20;
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, status);
    mouse_write(0xF6);
    mouse_read();
    mouse_write(0xF4);
    mouse_read();
}