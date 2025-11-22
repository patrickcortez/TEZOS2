#include "mbr.h"
#include "ata.h"
#include "video.h"

static mbr_t mbr;
static u32 partition_offsets[4];

void mbr_init() {
    /* Read MBR from sector 0 */
    u8 buffer[512];
    ata_read_sector(0, buffer);
    
    /* Copy to MBR structure */
    mbr = *(mbr_t*)buffer;
    
    /* Verify signature */
    if (mbr.signature != 0xAA55) {
        print_str("Invalid MBR signature!\n");
        return;
    }
    
    print_str("MBR detected. Partitions:\n");
    for (int i = 0; i < 4; i++) {
        partition_offsets[i] = mbr.partitions[i].first_lba;
        if (mbr.partitions[i].partition_type != 0) {
            print_str("  Partition ");
            print_char('0' + i);
            print_str(": LBA=");
            /* Print hex would be nice here, but keep simple */
            print_str(" Type=");
            print_char(mbr.partitions[i].partition_type < 10 ? 
                       '0' + mbr.partitions[i].partition_type : 
                       'A' + (mbr.partitions[i].partition_type - 10));
            print_str("\n");
        }
    }
}

u32 mbr_get_partition_start(int partition_num) {
    if (partition_num < 0 || partition_num >= 4) return 0;
    return partition_offsets[partition_num];
}

u32 mbr_get_partition_size(int partition_num) {
    if (partition_num < 0 || partition_num >= 4) return 0;
    return mbr.partitions[partition_num].sector_count;
}

void mbr_read_partition_table(partition_entry_t* out_entries) {
    /* Read MBR */
    u8 buffer[512];
    ata_read_sector(0, buffer);
    
    mbr_t* m = (mbr_t*)buffer;
    
    /* Copy partition entries */
    for (int i = 0; i < 4; i++) {
        out_entries[i] = m->partitions[i];
    }
}
