#ifndef INSTALLER_H
#define INSTALLER_H

#include "types.h"

/* --- Constants --- */
#define INSTALLER_VERSION "2.4.0-stable"
#define SECTOR_SIZE 512
#define MBR_BOOT_SIG 0xAA55
#define MAX_PATH_LEN 64

/* --- Partition Types --- */
#define PTYPE_EMPTY 0x00
#define PTYPE_FAT32 0x0B
#define PTYPE_LINUX 0x83
#define PTYPE_SWAP  0x82

/* --- Device Structures --- */
typedef enum {
    DEV_TYPE_UNKNOWN = 0,
    DEV_TYPE_IDE     = 1,
    DEV_TYPE_SATA    = 2,
    DEV_TYPE_NVME    = 3,
    DEV_TYPE_USB     = 4
} dev_type_t;

typedef struct {
    dev_type_t type;       // Hardware Interface
    u8         controller; // Controller ID (e.g., 0 for nvme0)
    u8         disk_id;    // Disk/Namespace ID (e.g., 1 for n1)
    u8         part_id;    // Partition Index (1-4 for MBR), 0 = Whole Disk
} device_map_t;

typedef struct {
    char source[MAX_PATH_LEN];  // "/dev/nvme0n1p2"
    char target[MAX_PATH_LEN];  // "/mnt"
    char fstype[16];            // "cortezfs"
    u8   mounted;
} mount_info_t;

/* --- Public API --- */
void installer_main(void);
int  parse_device_path(const char* path, device_map_t* out_map);

#endif