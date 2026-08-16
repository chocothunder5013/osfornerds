#include "font.h"
extern void put_pixel(int x, int y, uint32_t color);

/*
 * Renders a single 8x8 character to the framebuffer at (x, y).
 * Uses the font8x8_basic array to look up the bitmap pattern. It loops 
 * through each row (byte) and column (bit), calling put_pixel for every 
 * set bit. The column rendering is reversed (7 - col) because the most 
 * significant bit in each byte represents the leftmost pixel of the glyph.
 */
void        draw_char(int x, int y, char c, uint32_t color) {
    if (c < 0)
        return;
    const uint8_t *glyph = font8x8_basic[(int)c];
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (glyph[row] & (1 << col)) {
                put_pixel(x + (7 - col), y + row, color);
            }
        }
    }
}

/*
 * Draws a null-terminated string to the screen.
 * Iterates through the string and calls draw_char for each character, 
 * advancing the horizontal cursor by 10 pixels (8 for the character plus 
 * 2 for spacing) after each draw.
 */
void draw_string(int x, int y, const char *str, uint32_t color) {
    int cursor_x = x;
    while (*str) {
        draw_char(cursor_x, y, *str, color);
        cursor_x += 10;
        str++;
    }
}