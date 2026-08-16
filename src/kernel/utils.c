#include <stdint.h>
extern void serial_log(char *str);
/*
 * Formats a 32-bit unsigned integer as a hexadecimal string and outputs it to the serial port.
 * It uses bitwise shifts and masking to extract each nibble.
 */
void        serial_print_hex(uint32_t n) {
    char hex_chars[] = "0123456789ABCDEF";
    char buffer[11];
    buffer[0]  = '0';
    buffer[1]  = 'x';
    buffer[10] = '\0';
    for (int i = 0; i < 8; i++) {
        buffer[9 - i] = hex_chars[n & 0xF];
        n >>= 4;
    }
    serial_log(buffer);
}
/*
 * Formats a 32-bit unsigned integer as a base-10 string and outputs it to the serial port.
 * It handles the zero case explicitly and builds the string backwards.
 */
void serial_print_dec(uint32_t n) {
    if (n == 0) {
        serial_log("0");
        return;
    }
    char buffer[12];
    int  i     = 10;
    buffer[11] = 0;
    while (n > 0) {
        buffer[i--] = (n % 10) + '0';
        n /= 10;
    }
    serial_log(&buffer[i + 1]);
}
/*
 * Returns the length of a null-terminated string.
 */
int strlen(const char *str) {
    int len = 0;
    while (str[len])
        len++;
    return len;
}
/*
 * Compares two null-terminated strings lexicographically.
 * Returns 0 if they match, or the difference between the first non-matching characters.
 */
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}
/*
 * Copies a null-terminated string from src to dest.
 * It includes a null check to prevent kernel panics on null pointers, but assumes
 * the destination buffer is large enough to hold the source string.
 */
void strcpy_safe(char *dest, const char *src) {
    if (!dest || !src)
        return;
    int i = 0;
    while (src[i]) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = 0;
}