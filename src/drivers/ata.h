#ifndef ATA_H
#define ATA_H
#include <stdint.h>

/*
 * Initializes the primary master ATA drive.
 * This function communicates with the ATA controller via I/O ports to 
 * verify the presence of a drive and prepare it for read/write operations.
 */
void init_ata();

/*
 * Reads sectors from the ATA drive into a memory buffer.
 * It uses 28-bit Logical Block Addressing (LBA) to locate the sector, 
 * issues a read command (PIO mode), and polls the controller until the data 
 * is ready to be transferred to the target buffer.
 * 
 * lba: The 28-bit logical block address of the starting sector.
 * count: Number of sectors to read.
 * target: Memory location to store the read data.
 */
void ata_read_sectors(uint32_t lba, uint8_t count, uint32_t *target);

/*
 * Writes sectors from a memory buffer to the ATA drive.
 * Similar to reading, it uses PIO mode and 28-bit LBA. The function 
 * waits for the drive to become ready, transfers the data to the ATA 
 * data port, and flushes the drive's cache to ensure data is committed to disk.
 * 
 * lba: The 28-bit logical block address of the starting sector.
 * count: Number of sectors to write.
 * source: Memory location containing the data to write.
 */
void ata_write_sectors(uint32_t lba, uint8_t count, uint32_t *source);
#endif