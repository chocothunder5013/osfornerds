#ifndef SERIAL_H
#define SERIAL_H
#include <stdint.h>

/*
 * Initializes the COM1 serial port for 8N1 communication.
 * Configures the UART (Universal Asynchronous Receiver-Transmitter) speed,
 * enables FIFOs, and sets up interrupt line parameters.
 */
void    init_serial();

/*
 * Transmits a null-terminated string to the serial port.
 * Useful for kernel-level debugging and log output without relying on VGA.
 */
void    serial_log(char *str);

/*
 * Writes a single character to the serial port.
 * Waits for the transmit buffer to empty before sending.
 */
void    serial_write_char(char c);

void    outb(uint16_t port, uint8_t val);
uint8_t inb(uint16_t port);
#endif