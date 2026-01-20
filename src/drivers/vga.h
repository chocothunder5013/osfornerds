/* src/drivers/vga.h */
#ifndef VGA_H
#define VGA_H

#include <stdint.h>

void vga_clear();
void vga_print(const char* str);
void vga_putc(char c);

#endif