#include "ata.h"
#include "io.h"

#define ATA_DATA        0x1F0
#define ATA_ERROR       0x1F1
#define ATA_SECTOR_CNT  0x1F2
#define ATA_LBA_LOW     0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HIGH    0x1F5
#define ATA_DRIVE_HEAD  0x1F6
#define ATA_STATUS      0x1F7
#define ATA_COMMAND     0x1F7

static u32 partition_offset = 0;

void ata_set_partition_offset(u32 offset) {
    partition_offset = offset;
}

void ata_wait_bsy() {
    while (inb(ATA_STATUS) & 0x80);
}

void ata_wait_drq() {
    while (!(inb(ATA_STATUS) & 0x08));
}

void ata_read_sector(u32 lba, u8* buffer) {
    u32 actual_lba = lba + partition_offset;
    
    ata_wait_bsy();
    outb(ATA_DRIVE_HEAD, 0xE0 | ((actual_lba >> 24) & 0x0F));
    outb(ATA_SECTOR_CNT, 1);
    outb(ATA_LBA_LOW, (u8)actual_lba);
    outb(ATA_LBA_MID, (u8)(actual_lba >> 8));
    outb(ATA_LBA_HIGH, (u8)(actual_lba >> 16));
    outb(ATA_COMMAND, 0x20); /* Read Sectors */

    ata_wait_bsy();
    ata_wait_drq();

    for (int i = 0; i < 256; i++) {
        u16 data = inw(ATA_DATA);
        buffer[i * 2] = (u8)data;
        buffer[i * 2 + 1] = (u8)(data >> 8);
    }
}

void ata_write_sector(u32 lba, u8* buffer) {
    u32 actual_lba = lba + partition_offset;
    
    ata_wait_bsy();
    outb(ATA_DRIVE_HEAD, 0xE0 | ((actual_lba >> 24) & 0x0F));
    outb(ATA_SECTOR_CNT, 1);
    outb(ATA_LBA_LOW, (u8)actual_lba);
    outb(ATA_LBA_MID, (u8)(actual_lba >> 8));
    outb(ATA_LBA_HIGH, (u8)(actual_lba >> 16));
    outb(ATA_COMMAND, 0x30); /* Write Sectors */

    ata_wait_bsy();
    ata_wait_drq();

    for (int i = 0; i < 256; i++) {
        u16 data = buffer[i * 2] | (buffer[i * 2 + 1] << 8);
        outw(ATA_DATA, data);
    }
    
    outb(ATA_COMMAND, 0xE7); /* Cache Flush */
    ata_wait_bsy();
}
