#ifndef VGA_H
#define VGA_H
#include <stdint.h>

/*
 * Clears the entire VGA text buffer by writing the space character 
 * with the current background color to all character cells. Resets 
 * the hardware cursor to the top-left (0,0).
 */
void vga_clear();

/*
 * Outputs a null-terminated string to the VGA text display.
 * Iterates through the string and calls vga_putc for each character.
 */
void vga_print(const char *str);

/*
 * Outputs a single character to the VGA text display.
 * Handles special characters like newline (\n) and backspace (\b).
 * Automatically scrolls the terminal when the bottom of the screen is reached.
 */
void vga_putc(char c);
#endif