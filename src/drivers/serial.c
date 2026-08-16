#include "serial.h"
#define COM1 0x3F8
void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}
uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/*
 * Initializes the COM1 serial port (I/O port 0x3F8).
 * Configures the divisor latch to set the baud rate, sets data framing to 8N1
 * (8 data bits, no parity, 1 stop bit), enables the FIFO buffer, and configures
 * the modem control register.
 */
void init_serial() {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
}

/*
 * Reads the line status register (COM1 + 5) to check if the transmit holding
 * register is empty (bit 5). Returns non-zero if we can send another character.
 */
int is_transmit_empty() {
    return inb(COM1 + 5) & 0x20;
}

/*
 * Polls the line status register until the transmit buffer is empty,
 * then outputs the character to the COM1 data port. A timeout prevents
 * infinite loops in case the serial hardware hangs.
 */
void serial_write_char(char a) {
    int timeout = 10000;
    while (is_transmit_empty() == 0 && timeout > 0) {
        timeout--;
    }
    outb(COM1, a);
}

/*
 * Iterates through a null-terminated string, transmitting each character
 * to the serial port. Returns immediately if the string pointer is null.
 */
void serial_log(char *str) {
    if (!str)
        return;
    for (int i = 0; str[i] != 0; i++) {
        serial_write_char(str[i]);
    }
}