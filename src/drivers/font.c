#include "font.h" 
extern void put_pixel(int x, int y, uint32_t color);

void draw_char(int x, int y, char c, uint32_t color) {
    // Safety check for array bounds
    if (c < 0) return; 

    const uint8_t* glyph = font8x8_basic[(int)c];

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            // Check if the bit at this column is set
            // (1 << col) or (0x80 >> col) depending on font orientation
            // Standard VGA is usually MSB left.
            if (glyph[row] & (1 << col)) {
                 put_pixel(x + (7-col), y + row, color);
            }
        }
    }
}

void draw_string(int x, int y, const char* str, uint32_t color) {
    int cursor_x = x;
    while (*str) {
        draw_char(cursor_x, y, *str, color);
        cursor_x += 10; // 8px char + 2px padding
        str++;
    }
}