#include <stdint.h>
#include "serial.h" // for outb/inb

// Time Structure
typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} rtc_time_t;

int rtc_updating() {
    outb(0x70, 0x0A);
    return (inb(0x71) & 0x80);
}

uint8_t read_register(int reg) {
    outb(0x70, reg);
    return inb(0x71);
}

// Convert BCD to Binary (e.g., 0x15 -> 15)
uint8_t bcd_to_bin(uint8_t val) {
    return (val & 0x0F) + ((val / 16) * 10);
}

void get_rtc_time(rtc_time_t* t) {
    // Wait until RTC is stable (not updating)
    while (rtc_updating());

    uint8_t sec = read_register(0x00);
    uint8_t min = read_register(0x02);
    uint8_t hour = read_register(0x04);
    uint8_t day = read_register(0x07);
    uint8_t month = read_register(0x08);
    uint8_t year = read_register(0x09);
    uint8_t registerB = read_register(0x0B);

    // Convert BCD if necessary (Check bit 2 of Register B)
    if (!(registerB & 0x04)) {
        t->second = bcd_to_bin(sec);
        t->minute = bcd_to_bin(min);
        // Handle 12-hour clock bit (0x80) if necessary (omitted for brevity)
        t->hour   = bcd_to_bin(hour); 
        t->day    = bcd_to_bin(day);
        t->month  = bcd_to_bin(month);
        t->year   = bcd_to_bin(year) + 2000; // Assume 21st century
    } else {
        t->second = sec;
        t->minute = min;
        t->hour   = hour;
        t->day    = day;
        t->month  = month;
        t->year   = year + 2000;
    }
}