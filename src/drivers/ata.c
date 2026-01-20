/* src/drivers/ata.c */
#include "ata.h"
#include "serial.h"

// Extern scheduler
extern void schedule();

#define ATA_DATA        0x1F0
#define ATA_ERROR       0x1F1
#define ATA_SEC_COUNT   0x1F2
#define ATA_LBA_LO      0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HI      0x1F5
#define ATA_DRIVE_HEAD  0x1F6
#define ATA_STATUS      0x1F7
#define ATA_COMMAND     0x1F7

#define CMD_READ_PIO    0x20
#define CMD_WRITE_PIO   0x30
#define CMD_FLUSH_CACHE 0xE7
#define STATUS_BSY      0x80
#define STATUS_DRQ      0x08 
#define STATUS_ERR      0x01

// Simple spinlock for ATA driver to prevent race conditions
static volatile int ata_lock = 0;

void ata_acquire() {
    while (__sync_lock_test_and_set(&ata_lock, 1)) {
        schedule(); // Yield while waiting for lock
    }
}

void ata_release() {
    __sync_lock_release(&ata_lock);
}

void ata_wait_busy() {
    // Wait until BSY clears
    while (inb(ATA_STATUS) & STATUS_BSY) {
        schedule();
    }
}

void ata_wait_drq() {
    // Wait until DRQ sets or ERR sets
    while (!(inb(ATA_STATUS) & STATUS_DRQ)) {
        if (inb(ATA_STATUS) & STATUS_ERR) return; // Bail on error
        schedule();
    }
}

uint16_t insw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void outsw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

void init_ata() {
    ata_acquire();
    outb(ATA_DRIVE_HEAD, 0xA0);
    outb(ATA_SEC_COUNT, 0);
    outb(ATA_LBA_LO, 0);
    outb(ATA_LBA_MID, 0);
    outb(ATA_LBA_HI, 0);
    outb(ATA_COMMAND, 0xEC);
    uint8_t status = inb(ATA_STATUS);
    if (status == 0) {
        serial_log(" [ATA] No drive found.\n");
        ata_release();
        return;
    }
    
    ata_wait_busy();
    for(int i = 0; i < 256; i++) {
        insw(ATA_DATA);
    }
    serial_log(" [ATA] Primary Master Drive initialized.\n");
    ata_release();
}

void ata_read_sectors(uint32_t lba, uint8_t count, uint32_t* target) {
    ata_acquire();
    
    ata_wait_busy();
    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SEC_COUNT, count);
    outb(ATA_LBA_LO, (uint8_t) lba);
    outb(ATA_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_LBA_HI, (uint8_t)(lba >> 16));
    outb(ATA_COMMAND, CMD_READ_PIO);

    uint16_t* t = (uint16_t*) target;
    for (int j = 0; j < count; j++) {
        ata_wait_busy();
        ata_wait_drq();
        for (int i = 0; i < 256; i++) {
            t[i] = insw(ATA_DATA);
        }
        t += 256;
    }
    
    ata_release();
}

void ata_write_sectors(uint32_t lba, uint8_t count, uint32_t* source) {
    ata_acquire();
    
    ata_wait_busy();
    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SEC_COUNT, count);
    outb(ATA_LBA_LO, (uint8_t) lba);
    outb(ATA_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_LBA_HI, (uint8_t)(lba >> 16));
    outb(ATA_COMMAND, CMD_WRITE_PIO);

    uint16_t* s = (uint16_t*) source;
    for (int j = 0; j < count; j++) {
        ata_wait_busy();
        ata_wait_drq();
        for (int i = 0; i < 256; i++) {
            outsw(ATA_DATA, s[i]);
        }
        outb(ATA_COMMAND, CMD_FLUSH_CACHE);
        s += 256;
    }
    
    ata_release();
}