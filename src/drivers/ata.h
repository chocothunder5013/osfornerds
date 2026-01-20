#ifndef ATA_H
#define ATA_H

#include <stdint.h>

// Initialize ATA driver (identifies drive)
void init_ata();

// Read sectors from disk into a buffer
// target: Buffer to store data
// lba: Logical Block Address (Sector number, starts at 0)
// sector_count: How many sectors to read
void ata_read_sectors(uint32_t lba, uint8_t count, uint32_t* target);

// Write sectors from a buffer to disk
void ata_write_sectors(uint32_t lba, uint8_t count, uint32_t* source);

#endif