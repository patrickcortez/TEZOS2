#ifndef MBR_H
#define MBR_H

#include "types.h"

typedef struct {
    u8 status;
    u8 first_chs[3];
    u8 partition_type;
    u8 last_chs[3];
    u32 first_lba;
    u32 sector_count;
} __attribute__((packed)) partition_entry_t;

typedef struct {
    u8 bootstrap[446];
    partition_entry_t partitions[4];
    u16 signature;
} __attribute__((packed)) mbr_t;

void mbr_init();
u32 mbr_get_partition_start(int partition_num);
u32 mbr_get_partition_size(int partition_num);
void mbr_read_partition_table(partition_entry_t* out_entries);

#endif
