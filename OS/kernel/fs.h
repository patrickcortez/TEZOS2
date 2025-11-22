#ifndef FS_H
#define FS_H

#include "types.h"

/* ExFAT Constants */
#define EXFAT_SECTOR_SIZE 512
#define EXFAT_CLUSTER_SIZE 4096  /* 4KB clusters (8 sectors) */

/* Entry Types */
#define EXFAT_ENTRY_BITMAP      0x81
#define EXFAT_ENTRY_UPCASE      0x82
#define EXFAT_ENTRY_LABEL       0x83
#define EXFAT_ENTRY_FILE        0x85
#define EXFAT_ENTRY_INFO        0xC0
#define EXFAT_ENTRY_NAME        0xC1

/* File Attributes */
#define EXFAT_ATTR_READ_ONLY    0x01
#define EXFAT_ATTR_HIDDEN       0x02
#define EXFAT_ATTR_SYSTEM       0x04
#define EXFAT_ATTR_DIRECTORY    0x10
#define EXFAT_ATTR_ARCHIVE      0x20

/* File Open Flags */
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2
#define O_CREAT  4
#define O_TRUNC  8
#define O_APPEND 16
#define O_EXCL   32

/* Seek Flags */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/* Error Codes */
#define FS_SUCCESS       0
#define FS_ERR_NOT_FOUND -1
#define FS_ERR_INVALID   -2
#define FS_ERR_EXISTS    -3
#define FS_ERR_FULL      -4
#define FS_ERR_BUSY      -5
#define FS_ERR_IS_DIR    -6
#define FS_ERR_NOT_DIR   -7

/* ExFAT Boot Sector */
typedef struct {
    u8  jump_boot[3];
    u8  fs_name[8];         /* "EXFAT   " */
    u8  zero[53];
    u64 partition_offset;
    u64 volume_length;      /* In sectors */
    u32 fat_offset;         /* In sectors */
    u32 fat_length;         /* In sectors */
    u32 cluster_heap_offset;
    u32 cluster_count;
    u32 root_dir_cluster;
    u32 volume_serial;
    u16 fs_revision;
    u16 volume_flags;
    u8  bytes_per_sector_shift;
    u8  sectors_per_cluster_shift;
    u8  fat_count;
    u8  drive_select;
    u8  percent_in_use;
    u8  reserved[7];
    u8  boot_code[390];
    u16 signature;          /* 0xAA55 */
} __attribute__((packed)) exfat_boot_sector_t;

/* Generic Directory Entry (32 bytes) */
typedef struct {
    u8 type;
    u8 data[31];
} __attribute__((packed)) exfat_entry_t;

/* File Directory Entry (0x85) */
typedef struct {
    u8  type;               /* 0x85 */
    u8  secondary_count;
    u16 set_checksum;
    u16 file_attributes;
    u16 reserved1;
    u32 create_timestamp;
    u32 last_modified_timestamp;
    u32 last_accessed_timestamp;
    u8  create_10ms;
    u8  last_modified_10ms;
    u8  create_tz;
    u8  last_modified_tz;
    u8  last_accessed_tz;
    u8  reserved2[7];
} __attribute__((packed)) exfat_file_entry_t;

/* Stream Extension Entry (0xC0) */
typedef struct {
    u8  type;               /* 0xC0 */
    u8  flags;              /* Bit 1: No FAT Chain */
    u8  reserved1;
    u8  name_length;
    u16 name_hash;
    u16 reserved2;
    u64 valid_data_length;
    u64 reserved3;
    u32 first_cluster;
    u64 data_length;
} __attribute__((packed)) exfat_stream_entry_t;

/* File Name Entry (0xC1) */
typedef struct {
    u8  type;               /* 0xC1 */
    u8  flags;
    u16 name[15];           /* Unicode characters */
} __attribute__((packed)) exfat_name_entry_t;

/* Bitmap Entry (0x81) */
typedef struct {
    u8  type;               /* 0x81 */
    u8  flags;
    u8  reserved[18];
    u32 first_cluster;
    u64 data_length;
} __attribute__((packed)) exfat_bitmap_entry_t;

/* Upcase Table Entry (0x82) */
typedef struct {
    u8  type;               /* 0x82 */
    u8  reserved1[3];
    u32 checksum;
    u8  reserved2[12];
    u32 first_cluster;
    u64 data_length;
} __attribute__((packed)) exfat_upcase_entry_t;

/* File Handle */
typedef struct {
    u32 first_cluster;
    u32 current_cluster;
    u64 current_offset;     /* Global offset in file */
    u32 cluster_offset;     /* Offset within current cluster */
    u64 size;
    int flags;
    int dirty;
    u32 dir_cluster;        /* Cluster of directory containing this file */
    int dir_index;          /* Index in directory */
    int is_contiguous;      /* 1 if NoFatChain */
    int is_directory;       /* 1 if directory */
} fs_file_t;

#define O_DIRECTORY 0x10000

/* Directory Handle */
typedef struct {
    u32 first_cluster;
    u32 current_cluster;
    int current_index;
} fs_dir_t;

/* Directory Entry Info (Internal/Public) */
typedef struct {
    char name[256];
    u8 is_directory;
    u32 first_cluster;
    u64 size;
} dir_entry_t;

/* File Info (Public) */
typedef struct {
    char name[256];
    u64 size;
    u8 is_directory;
    u32 created;
    u32 modified;
} file_info_t;

/* Public API */
void fs_init(void);
void fs_format(void);

/* Handle-based File Operations */
fs_file_t* fs_open(const char* path, int flags);
int fs_close(fs_file_t* file);
int fs_read(fs_file_t* file, void* buffer, u32 size);
int fs_write(fs_file_t* file, const void* buffer, u32 size);
int fs_seek(fs_file_t* file, int offset, int whence);
int fs_tell(fs_file_t* file);
int fs_flush(fs_file_t* file);

/* Directory Operations */
fs_dir_t* fs_opendir(const char* path);
int fs_closedir(fs_dir_t* dir);
int fs_readdir(fs_dir_t* dir, dir_entry_t* entry);
int fs_readdir_file(fs_file_t* file, dir_entry_t* entry);

/* Path-based Helpers */
int fs_mkdir(const char* path);
int fs_rmdir(const char* path);
int fs_create(const char* path);
int fs_delete(const char* path);
int fs_stat(const char* path, file_info_t* info);
int fs_rename(const char* old_path, const char* new_path);
int fs_exists(const char* path);
int fs_chdir(const char* path);
int fs_getcwd(char* buffer, u32 size);

/* Compatibility Wrappers */
int fs_read_file(const char* path, void* buffer, u32 offset, u32 size);
int fs_write_file(const char* path, const void* buffer, u32 offset, u32 size);

/* Debug */
void fs_print_tree(const char* path, int level);

#endif
