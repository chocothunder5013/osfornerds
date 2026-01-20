/* src/drivers/vga.c */
#include <stdint.h>
#include <stddef.h>
#include "serial.h" 
#include "vga.h"

// Video Memory
volatile uint16_t* vga_buffer = (uint16_t*)0xB8000;
const int VGA_COLS = 80;
const int VGA_ROWS = 25;

int term_col = 0;
int term_row = 0;
uint8_t term_color = 0x0F; 

void vga_update_cursor(int x, int y) {
	uint16_t pos = y * VGA_COLS + x;
	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

void vga_clear() {
    for (int col = 0; col < VGA_COLS; col++) {
        for (int row = 0; row < VGA_ROWS; row++) {
            const size_t index = (row * VGA_COLS) + col;
            vga_buffer[index] = ((uint16_t)term_color << 8) | ' ';
        }
    }
    term_col = 0;
    term_row = 0;
    vga_update_cursor(0, 0);
}

void vga_scroll() {
    for (int row = 1; row < VGA_ROWS; row++) {
        for (int col = 0; col < VGA_COLS; col++) {
            int old_index = (row * VGA_COLS) + col;
            int new_index = ((row - 1) * VGA_COLS) + col;
            vga_buffer[new_index] = vga_buffer[old_index];
        }
    }
    for (int col = 0; col < VGA_COLS; col++) {
        int index = ((VGA_ROWS - 1) * VGA_COLS) + col;
        vga_buffer[index] = ((uint16_t)term_color << 8) | ' ';
    }
    term_row = VGA_ROWS - 1;
}

void vga_putc(char c) {
    if (c == '\n') {
        term_col = 0;
        term_row++;
    } 
    else if (c == '\b') {
        if (term_col > 0) term_col--;
        int index = (term_row * VGA_COLS) + term_col;
        vga_buffer[index] = ((uint16_t)term_color << 8) | ' ';
    }
    else {
        int index = (term_row * VGA_COLS) + term_col;
        vga_buffer[index] = ((uint16_t)term_color << 8) | c;
        term_col++;
    }

    if (term_col >= VGA_COLS) {
        term_col = 0;
        term_row++;
    }
    if (term_row >= VGA_ROWS) {
        vga_scroll();
    }
    
    vga_update_cursor(term_col, term_row);
}

void vga_print(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        vga_putc(str[i]);
    }
}