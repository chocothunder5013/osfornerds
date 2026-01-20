/* src/drivers/serial.c */
#include "serial.h"

#define COM1 0x3F8

void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

void init_serial() {
    outb(COM1 + 1, 0x00);    // Disable interrupts
    outb(COM1 + 3, 0x80);    // Enable DLAB
    outb(COM1 + 0, 0x03);    // 38400 baud
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);    // 8 bits, no parity
    outb(COM1 + 2, 0xC7);    // FIFO
    outb(COM1 + 4, 0x0B);    // IRQs enabled
}

int is_transmit_empty() {
    return inb(COM1 + 5) & 0x20;
}

void serial_write_char(char a) {
    int timeout = 10000;
    while (is_transmit_empty() == 0 && timeout > 0) {
        timeout--;
    }
    outb(COM1, a);
}

void serial_log(char *str) {
    if (!str) return;
    for (int i = 0; str[i] != 0; i++) {
        serial_write_char(str[i]);
    }
}