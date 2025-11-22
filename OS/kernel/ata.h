#ifndef ATA_H
#define ATA_H

#include "types.h"

void ata_set_partition_offset(u32 offset);
void ata_read_sector(u32 lba, u8* buffer);
void ata_write_sector(u32 lba, u8* buffer);

#endif
