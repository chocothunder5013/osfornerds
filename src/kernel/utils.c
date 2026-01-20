/* src/kernel/utils.c */
#include <stdint.h>

// FIX: Tell the compiler this function exists in another file (serial.c)
extern void serial_log(char *str);

// Helper: Convert integer to Hex String and send to Serial
void serial_print_hex(uint32_t n) {
    char hex_chars[] = "0123456789ABCDEF";
    char buffer[11]; // "0x" + 8 chars + null
    buffer[0] = '0';
    buffer[1] = 'x';
    buffer[10] = '\0';

    for (int i = 0; i < 8; i++) {
        buffer[9 - i] = hex_chars[n & 0xF];
        n >>= 4;
    }
    
    serial_log(buffer);
}

// Helper: Print a decimal number
void serial_print_dec(uint32_t n) {
    if (n == 0) {
        serial_log("0");
        return;
    }

    char buffer[12];
    int i = 10;
    buffer[11] = 0;

    while (n > 0) {
        buffer[i--] = (n % 10) + '0';
        n /= 10;
    }
    
    serial_log(&buffer[i + 1]);
}
int strlen(const char* str) {
    int len = 0;
    while (str[len])
        len++;
    return len;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}
void strcpy_safe(char* dest, const char* src) {
    if (!dest || !src) return;
    int i = 0;
    while (src[i]) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = 0; // Null terminate
}