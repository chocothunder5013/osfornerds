#include "ata.h"
#include "serial.h"
extern void schedule();
#define ATA_DATA 0x1F0
#define ATA_ERROR 0x1F1
#define ATA_SEC_COUNT 0x1F2
#define ATA_LBA_LO 0x1F3
#define ATA_LBA_MID 0x1F4
#define ATA_LBA_HI 0x1F5
#define ATA_DRIVE_HEAD 0x1F6
#define ATA_STATUS 0x1F7
#define ATA_COMMAND 0x1F7
#define CMD_READ_PIO 0x20
#define CMD_WRITE_PIO 0x30
#define CMD_FLUSH_CACHE 0xE7
#define STATUS_BSY 0x80
#define STATUS_DRQ 0x08
#define STATUS_ERR 0x01
static volatile int ata_lock = 0;

/*
 * Acquires a spinlock to ensure mutually exclusive access to the ATA controller.
 * Since multiple threads might attempt disk I/O concurrently, we lock the
 * hardware to prevent command interleaving and data corruption.
 */
void                ata_acquire() {
    while (__sync_lock_test_and_set(&ata_lock, 1)) {
        schedule();
    }
}
void ata_release() {
    __sync_lock_release(&ata_lock);
}

/*
 * Polls the ATA status register until the drive clears its busy (BSY) flag.
 * The controller sets this flag while it processes a command or transfers data.
 */
void ata_wait_busy() {
    while (inb(ATA_STATUS) & STATUS_BSY) {
        schedule();
    }
}
/*
 * Polls the ATA status register waiting for the Data Request (DRQ) bit.
 * This indicates the drive has data ready to transfer to the CPU, or is 
 * ready to receive data from the CPU. Returns early if the Error (ERR) bit is set.
 */
void ata_wait_drq() {
    while (!(inb(ATA_STATUS) & STATUS_DRQ)) {
        if (inb(ATA_STATUS) & STATUS_ERR)
            return;
        schedule();
    }
}
uint16_t insw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
void outsw(uint16_t port, uint16_t val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}
/*
 * Initializes the primary master ATA drive.
 * Sends an IDENTIFY command (0xEC) to the ATA controller. If a drive is
 * present, we wait for it to clear the busy status and then read the 256
 * words of identification data to clear the buffer.
 */
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
    for (int i = 0; i < 256; i++) {
        insw(ATA_DATA);
    }
    serial_log(" [ATA] Primary Master Drive initialized.\n");
    ata_release();
}
/*
 * Reads a specified number of 512-byte sectors using PIO (Programmed I/O) mode.
 * We select the drive and provide the 28-bit Logical Block Address (LBA) in pieces 
 * to the LBA registers. After issuing the read command, we wait for DRQ and then
 * read the data port word by word (16 bits) into our target buffer.
 */
void ata_read_sectors(uint32_t lba, uint8_t count, uint32_t *target) {
    ata_acquire();
    ata_wait_busy();
    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SEC_COUNT, count);
    outb(ATA_LBA_LO, (uint8_t)lba);
    outb(ATA_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_LBA_HI, (uint8_t)(lba >> 16));
    outb(ATA_COMMAND, CMD_READ_PIO);
    uint16_t *t = (uint16_t *)target;
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
/*
 * Writes a specified number of 512-byte sectors using PIO mode.
 * Similar to reading, we supply the LBA and issue a write command. 
 * We wait for DRQ, write 256 words per sector to the data port, and issue
 * a cache flush command to ensure data commits to physical media.
 */
void ata_write_sectors(uint32_t lba, uint8_t count, uint32_t *source) {
    ata_acquire();
    ata_wait_busy();
    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SEC_COUNT, count);
    outb(ATA_LBA_LO, (uint8_t)lba);
    outb(ATA_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_LBA_HI, (uint8_t)(lba >> 16));
    outb(ATA_COMMAND, CMD_WRITE_PIO);
    uint16_t *s = (uint16_t *)source;
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