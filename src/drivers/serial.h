/* src/drivers/serial.h */
#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

// Initialize the serial port
void init_serial();

// Write a string to the serial port
void serial_log(char *str);

// Write a single character
void serial_write_char(char c);

// I/O Port wrappers (used by IDT and others)
void outb(uint16_t port, uint8_t val);
uint8_t inb(uint16_t port);

#endif