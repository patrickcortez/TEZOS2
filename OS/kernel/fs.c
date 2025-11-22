#include "fs.h"
#include "ata.h"
#include "string.h"
#include "serial.h"
#include "heap.h"
#include "video.h"

/* ExFAT Globals */
static exfat_boot_sector_t boot_sector;
static u32 fat_start_sector;
static u32 cluster_heap_start;
static u32 root_dir_cluster;
static u32 total_clusters;
static u32* fat_cache = 0;          /* Cache for FAT */
static u8* bitmap_cache = 0;        /* Cache for Allocation Bitmap */
static u32 bitmap_start_cluster = 0;
static u32 upcase_start_cluster = 0;

static char cwd[256] = "/";
static u32 cwd_cluster = 0;         /* 0 means uninitialized, will be set to root on init */
int fs_mounted = 0;

/* Constants */
#define SECTOR_SIZE 512
#define CLUSTER_SIZE 4096
#define SECTORS_PER_CLUSTER 8
#define FAT_EOF 0xFFFFFFFF
#define FAT_BAD 0xFFFFFFF7
#define FAT_FREE 0x00000000

/* Forward Declarations */
static u32 resolve_path(const char* path);
static u32 alloc_cluster(int count, int contiguous);
static void free_cluster_chain(u32 cluster);
static u32 get_next_cluster(u32 cluster);
static void set_next_cluster(u32 cluster, u32 next);
static void update_bitmap(u32 cluster, int used);
static int find_entry(u32 dir_cluster, const char* name, exfat_file_entry_t* file_entry, exfat_stream_entry_t* stream_entry, u32* entry_cluster, int* entry_index);
static int find_free_slot(u32 dir_cluster, int slots_needed, u32* result_cluster, int* result_index);

/* --- Low Level Helpers --- */

static u32 cluster_to_sector(u32 cluster) {
    if (cluster < 2) return 0;
    return cluster_heap_start + (cluster - 2) * SECTORS_PER_CLUSTER;
}

static void read_cluster(u32 cluster, void* buffer) {
    u32 sector = cluster_to_sector(cluster);
    for (int i = 0; i < SECTORS_PER_CLUSTER; i++) {
        ata_read_sector(sector + i, (u8*)buffer + i * SECTOR_SIZE);
    }
}

static void write_cluster(u32 cluster, const void* buffer) {
    u32 sector = cluster_to_sector(cluster);
    for (int i = 0; i < SECTORS_PER_CLUSTER; i++) {
        ata_write_sector(sector + i, (u8*)buffer + i * SECTOR_SIZE);
    }
}

/* --- FAT & Bitmap Operations --- */

static u32 get_next_cluster(u32 cluster) {
    if (cluster < 2 || cluster >= total_clusters + 2) return FAT_EOF;
    return fat_cache[cluster];
}

static void set_next_cluster(u32 cluster, u32 next) {
    if (cluster < 2 || cluster >= total_clusters + 2) return;
    fat_cache[cluster] = next;
    
    /* Write back FAT sector */
    u32 fat_sector = fat_start_sector + (cluster * 4) / SECTOR_SIZE;
    u32 offset = (cluster * 4) % SECTOR_SIZE;
    
    u8 sector_buf[SECTOR_SIZE];
    ata_read_sector(fat_sector, sector_buf);
    *(u32*)(sector_buf + offset) = next;
    ata_write_sector(fat_sector, sector_buf);
}

static void update_bitmap(u32 cluster, int used) {
    if (cluster < 2) return;
    u32 bit_index = cluster - 2;
    u32 byte_index = bit_index / 8;
    u32 bit_offset = bit_index % 8;
    
    if (used) bitmap_cache[byte_index] |= (1 << bit_offset);
    else bitmap_cache[byte_index] &= ~(1 << bit_offset);
    
    /* Write back bitmap sector */
    if (bitmap_start_cluster) {
        u32 sector_offset = byte_index / SECTOR_SIZE;
        u32 byte_offset_in_sector = byte_index % SECTOR_SIZE;
        u32 bitmap_sector = cluster_to_sector(bitmap_start_cluster) + sector_offset;
        
        u8 sector_buf[SECTOR_SIZE];
        ata_read_sector(bitmap_sector, sector_buf);
        sector_buf[byte_offset_in_sector] = bitmap_cache[byte_index];
        ata_write_sector(bitmap_sector, sector_buf);
    }
}

static u32 alloc_cluster(int count, int contiguous) {
    u32 start_cluster = 0;
    int found = 0;
    
    for (u32 i = 2; i < total_clusters + 2; i++) {
        u32 bit_index = i - 2;
        if (!(bitmap_cache[bit_index / 8] & (1 << (bit_index % 8)))) {
            if (start_cluster == 0) start_cluster = i;
            found++;
            if (found == count) break;
            if (contiguous && start_cluster != 0 && i != start_cluster + found - 1) {
                start_cluster = 0;
                found = 0;
            }
        } else {
            start_cluster = 0;
            found = 0;
        }
    }
    
    if (found == count) {
        for (int i = 0; i < count; i++) {
            u32 c = start_cluster + i;
            update_bitmap(c, 1);
            if (!contiguous) {
                set_next_cluster(c, (i == count - 1) ? FAT_EOF : c + 1);
            }
        }
        return start_cluster;
    }
    
    return 0; /* Full */
}

static void free_cluster_chain(u32 cluster) {
    u32 current = cluster;
    while (current != FAT_EOF && current != 0) {
        u32 next = get_next_cluster(current);
        update_bitmap(current, 0);
        set_next_cluster(current, 0);
        current = next;
    }
}

/* --- Initialization --- */

void fs_init() {
    serial_print("[FS] Initializing ExFAT...\n");
    
    /* Read Boot Sector */
    ata_read_sector(0, (u8*)&boot_sector);
    
    if (boot_sector.signature != 0xAA55) {
        serial_print("[FS] Invalid boot signature\n");
        return;
    }
    
    char name[9];
    memcpy(name, boot_sector.fs_name, 8);
    name[8] = 0;
    if (strcmp(name, "EXFAT   ") != 0) {
        serial_print("[FS] Not ExFAT\n");
        return;
    }
    
    fat_start_sector = boot_sector.fat_offset;
    cluster_heap_start = boot_sector.cluster_heap_offset;
    total_clusters = boot_sector.cluster_count;
    root_dir_cluster = boot_sector.root_dir_cluster;
    
    /* Load FAT */
    fat_cache = (u32*)kmalloc(boot_sector.fat_length * SECTOR_SIZE);
    if (!fat_cache) return;
    for (u32 i = 0; i < boot_sector.fat_length; i++) {
        ata_read_sector(fat_start_sector + i, (u8*)fat_cache + i * SECTOR_SIZE);
    }
    
    /* Find Bitmap in Root Dir */
    u8* cluster_buf = (u8*)kmalloc(CLUSTER_SIZE);
    read_cluster(root_dir_cluster, cluster_buf);
    
    exfat_entry_t* entry = (exfat_entry_t*)cluster_buf;
    for (int i = 0; i < CLUSTER_SIZE / 32; i++) {
        if (entry[i].type == EXFAT_ENTRY_BITMAP) {
            exfat_bitmap_entry_t* bmp = (exfat_bitmap_entry_t*)&entry[i];
            bitmap_start_cluster = bmp->first_cluster;
            break;
        }
    }
    
    if (bitmap_start_cluster) {
        u32 bitmap_sectors = (total_clusters + 7) / 8 / SECTOR_SIZE + 1;
        bitmap_cache = (u8*)kmalloc(bitmap_sectors * SECTOR_SIZE);
        u32 sector = cluster_to_sector(bitmap_start_cluster);
        for (u32 i = 0; i < bitmap_sectors; i++) {
            ata_read_sector(sector + i, bitmap_cache + i * SECTOR_SIZE);
        }
    }
    
    kfree(cluster_buf);
    cwd_cluster = root_dir_cluster;
    fs_mounted = 1;
    serial_print("[FS] ExFAT Mounted Successfully\n");
}

void fs_format() {
    serial_print("[FS] Formatting as ExFAT...\n");
    
    /* Zero out first 10MB */
    u8 zero[512];
    memset(zero, 0, 512);
    for (int i = 0; i < 20480; i++) ata_write_sector(i, zero);
    
    /* Boot Sector */
    memset(&boot_sector, 0, sizeof(exfat_boot_sector_t));
    memcpy(boot_sector.jump_boot, "\xEB\x76\x90", 3);
    memcpy(boot_sector.fs_name, "EXFAT   ", 8);
    boot_sector.partition_offset = 0;
    boot_sector.volume_length = 20480;
    boot_sector.fat_offset = 128;
    boot_sector.fat_length = 256;
    boot_sector.cluster_heap_offset = 512;
    boot_sector.cluster_count = (20480 - 512) / 8;
    boot_sector.root_dir_cluster = 2;
    boot_sector.volume_serial = 0x12345678;
    boot_sector.fs_revision = 0x0100;
    boot_sector.bytes_per_sector_shift = 9;
    boot_sector.sectors_per_cluster_shift = 3;
    boot_sector.fat_count = 1;
    boot_sector.drive_select = 0x80;
    boot_sector.signature = 0xAA55;
    
    ata_write_sector(0, (u8*)&boot_sector);
    
    /* FAT */
    u32* fat = (u32*)kmalloc(boot_sector.fat_length * 512);
    memset(fat, 0, boot_sector.fat_length * 512);
    fat[0] = 0xFFFFFFF8; fat[1] = 0xFFFFFFFF;
    fat[2] = 0xFFFFFFFF; /* Root */
    fat[3] = 0xFFFFFFFF; /* Bitmap */
    fat[4] = 0xFFFFFFFF; /* Upcase */
    
    for (int i = 0; i < boot_sector.fat_length; i++) {
        ata_write_sector(boot_sector.fat_offset + i, (u8*)fat + i * 512);
    }
    kfree(fat);
    
    /* Bitmap */
    u32 bitmap_size = (boot_sector.cluster_count + 7) / 8;
    u8* bitmap = (u8*)kmalloc(bitmap_size);
    memset(bitmap, 0, bitmap_size);
    bitmap[0] = 0x07; /* Clusters 2,3,4 used */
    
    u32 bitmap_sector = cluster_to_sector(3);
    ata_write_sector(bitmap_sector, bitmap);
    kfree(bitmap);
    
    /* Root Dir */
    u8* root_dir = (u8*)kmalloc(CLUSTER_SIZE);
    memset(root_dir, 0, CLUSTER_SIZE);
    
    exfat_bitmap_entry_t* bmp = (exfat_bitmap_entry_t*)root_dir;
    bmp->type = EXFAT_ENTRY_BITMAP;
    bmp->first_cluster = 3;
    bmp->data_length = bitmap_size;
    
    exfat_upcase_entry_t* upcase = (exfat_upcase_entry_t*)(root_dir + 32);
    upcase->type = EXFAT_ENTRY_UPCASE;
    upcase->first_cluster = 4;
    
    write_cluster(2, root_dir);
    kfree(root_dir);
    
    fs_init();
}

/* --- Directory Operations --- */

static int find_entry(u32 dir_cluster, const char* name, exfat_file_entry_t* file_entry, exfat_stream_entry_t* stream_entry, u32* entry_cluster, int* entry_index) {
    u8* buf = (u8*)kmalloc(CLUSTER_SIZE);
    u32 current = dir_cluster;
    
    while (current != FAT_EOF && current != 0) {
        read_cluster(current, buf);
        
        for (int i = 0; i < CLUSTER_SIZE / 32; i++) {
            exfat_entry_t* raw = (exfat_entry_t*)(buf + i * 32);
            if (raw->type == EXFAT_ENTRY_FILE) {
                /* Check if we have enough space for the set */
                if (i + 2 >= CLUSTER_SIZE / 32) break; /* Handle split entries later */
                
                exfat_name_entry_t* name_ent = (exfat_name_entry_t*)(buf + (i + 2) * 32);
                char entry_name[16];
                for(int k=0; k<15; k++) entry_name[k] = (char)name_ent->name[k];
                entry_name[15] = 0;
                
                if (strcmp(entry_name, name) == 0) {
                    memcpy(file_entry, raw, 32);
                    memcpy(stream_entry, buf + (i + 1) * 32, 32);
                    *entry_cluster = current;
                    *entry_index = i;
                    kfree(buf);
                    return 1;
                }
            }
        }
        current = get_next_cluster(current);
    }
    
    kfree(buf);
    return 0;
}

static int find_free_slot(u32 dir_cluster, int slots_needed, u32* result_cluster, int* result_index) {
    u8* buf = (u8*)kmalloc(CLUSTER_SIZE);
    u32 current = dir_cluster;
    u32 prev = 0;
    
    while (1) {
        read_cluster(current, buf);
        
        int consecutive = 0;
        int start_idx = -1;
        
        for (int i = 0; i < CLUSTER_SIZE / 32; i++) {
            exfat_entry_t* raw = (exfat_entry_t*)(buf + i * 32);
            if (raw->type == 0 || !(raw->type & 0x80)) { /* Free or Deleted */
                if (consecutive == 0) start_idx = i;
                consecutive++;
                if (consecutive == slots_needed) {
                    *result_cluster = current;
                    *result_index = start_idx;
                    kfree(buf);
                    return 1;
                }
            } else {
                consecutive = 0;
            }
        }
        
        prev = current;
        current = get_next_cluster(current);
        if (current == FAT_EOF || current == 0) {
            /* Allocate new cluster for directory */
            u32 new_cluster = alloc_cluster(1, 0);
            if (new_cluster == 0) {
                kfree(buf);
                return 0;
            }
            
            /* Clear new cluster */
            memset(buf, 0, CLUSTER_SIZE);
            write_cluster(new_cluster, buf);
            
            set_next_cluster(prev, new_cluster);
            current = new_cluster;
            /* Loop will catch it next time */
        }
    }
}

/* Helper to canonicalize path (resolve . and ..) */
static void canonicalize_path(const char* input, char* output) {
    char temp[256];
    int out_idx = 0;
    int in_idx = 0;
    
    /* Start with / */
    temp[out_idx++] = '/';
    
    if (input[0] == '/') in_idx++; /* Skip leading / */
    
    while (input[in_idx]) {
        /* Get next component */
        int start = in_idx;
        while (input[in_idx] && input[in_idx] != '/') in_idx++;
        int len = in_idx - start;
        
        if (len > 0) {
            if (len == 1 && input[start] == '.') {
                /* Ignore . */
            } else if (len == 2 && input[start] == '.' && input[start+1] == '.') {
                /* Handle .. : Backtrack output */
                if (out_idx > 1) {
                    out_idx--; /* Skip current slash */
                    while (out_idx > 0 && temp[out_idx-1] != '/') out_idx--;
                }
            } else {
                /* Append component */
                if (out_idx > 1) temp[out_idx++] = '/';
                for (int i = 0; i < len; i++) temp[out_idx++] = input[start+i];
            }
        }
        
        if (input[in_idx] == '/') in_idx++;
    }
    
    if (out_idx == 0) temp[out_idx++] = '/';
    temp[out_idx] = 0;
    
    strcpy(output, temp);
}

static u32 resolve_path(const char* path) {
    if (!path) return 0;
    
    char abs_path[256];
    char full_path[256];
    
    /* Construct full path string */
    if (path[0] == '/') {
        strcpy(full_path, path);
    } else {
        strcpy(full_path, cwd);
        if (strcmp(cwd, "/") != 0) strcat(full_path, "/");
        strcat(full_path, path);
    }
    
    /* Canonicalize */
    canonicalize_path(full_path, abs_path);
    
    /* Walk from root */
    u32 current_cluster = root_dir_cluster;
    if (strcmp(abs_path, "/") == 0) return root_dir_cluster;
    
    char* p = abs_path;
    if (*p == '/') p++;
    
    char component[256];
    int i = 0;
    
    while (*p) {
        if (*p == '/') {
            component[i] = 0;
            if (i > 0) {
                exfat_file_entry_t f; exfat_stream_entry_t s; u32 ec; int ei;
                if (find_entry(current_cluster, component, &f, &s, &ec, &ei)) {
                    if (f.file_attributes & EXFAT_ATTR_DIRECTORY) {
                        current_cluster = s.first_cluster;
                    } else {
                        return 0; /* Not a directory */
                    }
                } else {
                    return 0; /* Not found */
                }
            }
            i = 0;
        } else {
            component[i++] = *p;
        }
        p++;
    }
    
    /* Final component */
    if (i > 0) {
        component[i] = 0;
        exfat_file_entry_t f; exfat_stream_entry_t s; u32 ec; int ei;
        if (find_entry(current_cluster, component, &f, &s, &ec, &ei)) {
            if (f.file_attributes & EXFAT_ATTR_DIRECTORY) {
                current_cluster = s.first_cluster;
            } else {
                return 0;
            }
        } else {
            return 0;
        }
    }
    
    return current_cluster;
}

fs_dir_t* fs_opendir(const char* path) {
    if (!fs_mounted) return 0;
    u32 cluster = resolve_path(path);
    if (cluster == 0) return 0;
    
    fs_dir_t* dir = (fs_dir_t*)kmalloc(sizeof(fs_dir_t));
    dir->first_cluster = cluster;
    dir->current_cluster = cluster;
    dir->current_index = 0;
    return dir;
}

int fs_closedir(fs_dir_t* dir) {
    if (dir) kfree(dir);
    return 0;
}

int fs_readdir(fs_dir_t* dir, dir_entry_t* entry) {
    if (!dir || !entry) return -1;
    
    u8* buf = (u8*)kmalloc(CLUSTER_SIZE);
    
    while (1) {
        read_cluster(dir->current_cluster, buf);
        
        while (dir->current_index < CLUSTER_SIZE / 32) {
            exfat_entry_t* raw = (exfat_entry_t*)(buf + dir->current_index * 32);
            
            if (raw->type == 0) { /* End */
                kfree(buf);
                return -1;
            }
            
            if (raw->type == EXFAT_ENTRY_FILE) {
                /* Found file, parse set */
                if (dir->current_index + 2 >= CLUSTER_SIZE / 32) {
                    /* Set crosses cluster boundary - tricky, skip for now or handle */
                    dir->current_index++; 
                    continue; 
                }
                
                exfat_file_entry_t* file = (exfat_file_entry_t*)raw;
                exfat_stream_entry_t* stream = (exfat_stream_entry_t*)(buf + (dir->current_index + 1) * 32);
                exfat_name_entry_t* name = (exfat_name_entry_t*)(buf + (dir->current_index + 2) * 32);
                
                dir->current_index += 3; /* Advance past set */
                
                if (stream->type == EXFAT_ENTRY_INFO && name->type == EXFAT_ENTRY_NAME) {
                    memset(entry->name, 0, 256);
                    for(int i=0; i<15; i++) entry->name[i] = (char)name->name[i];
                    entry->is_directory = (file->file_attributes & EXFAT_ATTR_DIRECTORY) ? 1 : 0;
                    entry->first_cluster = stream->first_cluster;
                    entry->size = stream->data_length;
                    kfree(buf);
                    return 0;
                }
            } else {
                dir->current_index++;
            }
        }
        
        u32 next = get_next_cluster(dir->current_cluster);
        if (next == FAT_EOF || next == 0) {
            kfree(buf);
            return -1;
        }
        dir->current_cluster = next;
        dir->current_index = 0;
    }
}

/* --- File Operations --- */

int fs_create(const char* path) {
    /* Split path */
    char parent[256];
    char name[256];
    int len = strlen(path);
    int last_slash = -1;
    for(int i=len-1; i>=0; i--) {
        if(path[i] == '/') { last_slash = i; break; }
    }
    
    if (last_slash == -1) {
        strcpy(parent, cwd);
        strcpy(name, path);
    } else if (last_slash == 0) {
        strcpy(parent, "/");
        strcpy(name, path + 1);
    } else {
        strncpy(parent, path, last_slash);
        parent[last_slash] = 0;
        strcpy(name, path + last_slash + 1);
    }
    
    u32 dir_cluster = resolve_path(parent);
    if (dir_cluster == 0) return -1;
    
    /* Find free slot */
    u32 entry_cluster;
    int entry_index;
    if (!find_free_slot(dir_cluster, 3, &entry_cluster, &entry_index)) return -1;
    
    /* Prepare entries */
    u8* buf = (u8*)kmalloc(CLUSTER_SIZE);
    read_cluster(entry_cluster, buf);
    
    exfat_file_entry_t* file = (exfat_file_entry_t*)(buf + entry_index * 32);
    exfat_stream_entry_t* stream = (exfat_stream_entry_t*)(buf + (entry_index + 1) * 32);
    exfat_name_entry_t* name_ent = (exfat_name_entry_t*)(buf + (entry_index + 2) * 32);
    
    memset(file, 0, 32);
    file->type = EXFAT_ENTRY_FILE;
    file->secondary_count = 2;
    file->file_attributes = EXFAT_ATTR_ARCHIVE;
    
    memset(stream, 0, 32);
    stream->type = EXFAT_ENTRY_INFO;
    stream->first_cluster = 0;
    
    memset(name_ent, 0, 32);
    name_ent->type = EXFAT_ENTRY_NAME;
    for(int i=0; i<15 && name[i]; i++) name_ent->name[i] = name[i];
    
    write_cluster(entry_cluster, buf);
    kfree(buf);
    return 0;
}

int fs_mkdir(const char* path) {
    if (fs_create(path) != 0) return -1;
    
    /* Set directory attribute */
    /* Resolve again to find it */
    char parent[256];
    char name[256];
    /* (Same split logic as create) */
    int len = strlen(path);
    int last_slash = -1;
    for(int i=len-1; i>=0; i--) {
        if(path[i] == '/') { last_slash = i; break; }
    }
    if (last_slash == -1) { strcpy(parent, cwd); strcpy(name, path); }
    else if (last_slash == 0) { strcpy(parent, "/"); strcpy(name, path + 1); }
    else { strncpy(parent, path, last_slash); parent[last_slash] = 0; strcpy(name, path + last_slash + 1); }
    
    u32 dir_cluster = resolve_path(parent);
    exfat_file_entry_t f; exfat_stream_entry_t s;
    u32 ec; int ei;
    
    if (find_entry(dir_cluster, name, &f, &s, &ec, &ei)) {
        u8* buf = (u8*)kmalloc(CLUSTER_SIZE);
        read_cluster(ec, buf);
        exfat_file_entry_t* file = (exfat_file_entry_t*)(buf + ei * 32);
        file->file_attributes |= EXFAT_ATTR_DIRECTORY;
        
        /* Allocate cluster for new directory */
        u32 new_dir_cluster = alloc_cluster(1, 0);
        exfat_stream_entry_t* stream = (exfat_stream_entry_t*)(buf + (ei + 1) * 32);
        stream->first_cluster = new_dir_cluster;
        stream->data_length = CLUSTER_SIZE;
        stream->valid_data_length = CLUSTER_SIZE;
        
        /* Clear new directory cluster */
        u8* zero = (u8*)kmalloc(CLUSTER_SIZE);
        memset(zero, 0, CLUSTER_SIZE);
        write_cluster(new_dir_cluster, zero);
        kfree(zero);
        
        write_cluster(ec, buf);
        kfree(buf);
        return 0;
    }
    return -1;
}

fs_file_t* fs_open(const char* path, int flags) {
    if (!fs_mounted) return 0;
    
    char parent[256];
    char name[256];
    int len = strlen(path);
    int last_slash = -1;
    for(int i=len-1; i>=0; i--) {
        if(path[i] == '/') { last_slash = i; break; }
    }
    
    if (last_slash == -1) {
        strcpy(parent, cwd);
        strcpy(name, path);
    } else if (last_slash == 0) {
        strcpy(parent, "/");
        strcpy(name, path + 1);
    } else {
        strncpy(parent, path, last_slash);
        parent[last_slash] = 0;
        strcpy(name, path + last_slash + 1);
    }
    
    u32 dir_cluster = resolve_path(parent);
    if (dir_cluster == 0) return 0;
    
    exfat_file_entry_t file_entry;
    exfat_stream_entry_t stream_entry;
    u32 entry_cluster;
    int entry_index;
    
    int found = find_entry(dir_cluster, name, &file_entry, &stream_entry, &entry_cluster, &entry_index);
    
    if (!found) {
        if (flags & O_CREAT) {
            if (fs_create(path) != 0) return 0;
            find_entry(dir_cluster, name, &file_entry, &stream_entry, &entry_cluster, &entry_index);
        } else {
            return 0;
        }
    } else {
        if ((flags & O_CREAT) && (flags & O_EXCL)) return 0;
        /* Allow opening directory if O_DIRECTORY is set or just O_RDONLY */
        /* But block writing to directory via fs_write */
    }
    
    fs_file_t* file = (fs_file_t*)kmalloc(sizeof(fs_file_t));
    file->first_cluster = stream_entry.first_cluster;
    file->current_cluster = stream_entry.first_cluster;
    file->current_offset = 0;
    file->cluster_offset = 0;
    file->size = stream_entry.data_length;
    file->flags = flags;
    file->dirty = 0;
    file->dir_cluster = entry_cluster;
    file->dir_index = entry_index;
    file->is_contiguous = (stream_entry.flags & 0x02);
    file->is_directory = (file_entry.file_attributes & EXFAT_ATTR_DIRECTORY) ? 1 : 0;
    
    if (file->is_directory && (flags & O_WRONLY || flags & O_RDWR)) {
        kfree(file);
        return 0; /* Cannot open dir for writing */
    }
    
    if (flags & O_TRUNC) {
        /* Free chain */
        if (file->first_cluster) free_cluster_chain(file->first_cluster);
        file->first_cluster = 0;
        file->size = 0;
        file->dirty = 1;
    }
    
    if (flags & O_APPEND) {
        fs_seek(file, 0, SEEK_END);
    }
    
    return file;
}

int fs_close(fs_file_t* file) {
    if (!file) return -1;
    fs_flush(file);
    kfree(file);
    return 0;
}

int fs_read(fs_file_t* file, void* buffer, u32 size) {
    if (!file) return -1;
    if (file->first_cluster == 0) return 0; /* Empty file */
    
    u32 bytes_read = 0;
    u8* buf = (u8*)kmalloc(CLUSTER_SIZE);
    
    while (bytes_read < size && file->current_offset < file->size) {
        read_cluster(file->current_cluster, buf);
        
        u32 chunk = CLUSTER_SIZE - file->cluster_offset;
        if (chunk > size - bytes_read) chunk = size - bytes_read;
        if (chunk > file->size - file->current_offset) chunk = file->size - file->current_offset;
        
        memcpy((u8*)buffer + bytes_read, buf + file->cluster_offset, chunk);
        
        file->current_offset += chunk;
        file->cluster_offset += chunk;
        bytes_read += chunk;
        
        if (file->cluster_offset >= CLUSTER_SIZE) {
            file->cluster_offset = 0;
            if (file->is_contiguous) file->current_cluster++;
            else file->current_cluster = get_next_cluster(file->current_cluster);
        }
    }
    kfree(buf);
    return bytes_read;
}

int fs_write(fs_file_t* file, const void* buffer, u32 size) {
    if (!file) return -1;
    
    u32 bytes_written = 0;
    u8* buf = (u8*)kmalloc(CLUSTER_SIZE);
    
    while (bytes_written < size) {
        if (file->first_cluster == 0) {
            file->first_cluster = alloc_cluster(1, 0);
            file->current_cluster = file->first_cluster;
        }
        
        read_cluster(file->current_cluster, buf);
        
        u32 chunk = CLUSTER_SIZE - file->cluster_offset;
        if (chunk > size - bytes_written) chunk = size - bytes_written;
        
        memcpy(buf + file->cluster_offset, (u8*)buffer + bytes_written, chunk);
        write_cluster(file->current_cluster, buf);
        
        file->current_offset += chunk;
        file->cluster_offset += chunk;
        bytes_written += chunk;
        
        if (file->current_offset > file->size) file->size = file->current_offset;
        file->dirty = 1;
        
        if (file->cluster_offset >= CLUSTER_SIZE) {
            file->cluster_offset = 0;
            u32 next = get_next_cluster(file->current_cluster);
            if (next == FAT_EOF || next == 0) {
                next = alloc_cluster(1, 0);
                set_next_cluster(file->current_cluster, next);
            }
            file->current_cluster = next;
        }
    }
    kfree(buf);
    return bytes_written;
}

int fs_seek(fs_file_t* file, int offset, int whence) {
    if (!file) return -1;
    u64 new_pos = 0;
    if (whence == SEEK_SET) new_pos = offset;
    else if (whence == SEEK_CUR) new_pos = file->current_offset + offset;
    else if (whence == SEEK_END) new_pos = file->size + offset;
    
    file->current_cluster = file->first_cluster;
    file->current_offset = 0;
    
    while (file->current_offset + CLUSTER_SIZE <= new_pos) {
        if (file->is_contiguous) file->current_cluster++;
        else file->current_cluster = get_next_cluster(file->current_cluster);
        file->current_offset += CLUSTER_SIZE;
    }
    file->cluster_offset = new_pos - file->current_offset;
    file->current_offset = new_pos;
    return 0;
}

int fs_tell(fs_file_t* file) {
    return file ? file->current_offset : -1;
}

int fs_flush(fs_file_t* file) {
    if (!file || !file->dirty) return 0;
    
    u8* buf = (u8*)kmalloc(CLUSTER_SIZE);
    read_cluster(file->dir_cluster, buf);
    
    exfat_stream_entry_t* stream = (exfat_stream_entry_t*)(buf + (file->dir_index + 1) * 32);
    stream->data_length = file->size;
    stream->valid_data_length = file->size;
    stream->first_cluster = file->first_cluster;
    
    write_cluster(file->dir_cluster, buf);
    kfree(buf);
    file->dirty = 0;
    return 0;
}

int fs_delete(const char* path) {
    char parent[256]; char name[256];
    /* Split path logic */
    int len = strlen(path);
    int last_slash = -1;
    for(int i=len-1; i>=0; i--) { if(path[i] == '/') { last_slash = i; break; } }
    if (last_slash == -1) { strcpy(parent, cwd); strcpy(name, path); }
    else if (last_slash == 0) { strcpy(parent, "/"); strcpy(name, path + 1); }
    else { strncpy(parent, path, last_slash); parent[last_slash] = 0; strcpy(name, path + last_slash + 1); }
    
    u32 dir_cluster = resolve_path(parent);
    exfat_file_entry_t f; exfat_stream_entry_t s; u32 ec; int ei;
    
    if (find_entry(dir_cluster, name, &f, &s, &ec, &ei)) {
        /* Free chain */
        if (s.first_cluster) free_cluster_chain(s.first_cluster);
        
        /* Mark entries as deleted */
        u8* buf = (u8*)kmalloc(CLUSTER_SIZE);
        read_cluster(ec, buf);
        
        /* Mark File, Stream, Name as deleted (bit 7 of type = 0) */
        ((exfat_entry_t*)(buf + ei * 32))->type &= ~0x80;
        ((exfat_entry_t*)(buf + (ei+1) * 32))->type &= ~0x80;
        ((exfat_entry_t*)(buf + (ei+2) * 32))->type &= ~0x80;
        
        write_cluster(ec, buf);
        kfree(buf);
        return 0;
    }
    return -1;
}

int fs_rmdir(const char* path) {
    return fs_delete(path); /* ExFAT rmdir is same as delete, just need to ensure empty? */
}

int fs_stat(const char* path, file_info_t* info) {
    char parent[256]; char name[256];
    int len = strlen(path);
    int last_slash = -1;
    for(int i=len-1; i>=0; i--) { if(path[i] == '/') { last_slash = i; break; } }
    if (last_slash == -1) { strcpy(parent, cwd); strcpy(name, path); }
    else if (last_slash == 0) { strcpy(parent, "/"); strcpy(name, path + 1); }
    else { strncpy(parent, path, last_slash); parent[last_slash] = 0; strcpy(name, path + last_slash + 1); }
    
    u32 dir_cluster = resolve_path(parent);
    exfat_file_entry_t f; exfat_stream_entry_t s; u32 ec; int ei;
    
    if (find_entry(dir_cluster, name, &f, &s, &ec, &ei)) {
        strcpy(info->name, name);
        info->size = s.data_length;
        info->is_directory = (f.file_attributes & EXFAT_ATTR_DIRECTORY) ? 1 : 0;
        return 0;
    }
    return -1;
}

int fs_rename(const char* old_path, const char* new_path) {
    /* 1. Resolve old path to find its directory and entry */
    char old_parent[256]; char old_name[256];
    int len = strlen(old_path);
    int last_slash = -1;
    for(int i=len-1; i>=0; i--) { if(old_path[i] == '/') { last_slash = i; break; } }
    
    if (last_slash == -1) { 
        strcpy(old_parent, cwd); 
        strcpy(old_name, old_path); 
    } else if (last_slash == 0) { 
        strcpy(old_parent, "/"); 
        strcpy(old_name, old_path + 1); 
    } else { 
        strncpy(old_parent, old_path, last_slash); 
        old_parent[last_slash] = 0; 
        strcpy(old_name, old_path + last_slash + 1); 
    }
    
    u32 old_dir_cluster = resolve_path(old_parent);
    if (old_dir_cluster == 0) return -1;

    exfat_file_entry_t f; 
    exfat_stream_entry_t s; 
    u32 old_entry_cluster; 
    int old_entry_index;
    
    if (!find_entry(old_dir_cluster, old_name, &f, &s, &old_entry_cluster, &old_entry_index)) {
        return -1; /* Old file not found */
    }
    
    /* 2. Check if new path already exists */
    if (fs_exists(new_path)) return -1; /* Target exists */
    
    /* 3. Resolve new path parent directory */
    char new_parent[256]; char new_name[256];
    len = strlen(new_path);
    last_slash = -1;
    for(int i=len-1; i>=0; i--) { if(new_path[i] == '/') { last_slash = i; break; } }
    
    if (last_slash == -1) { 
        strcpy(new_parent, cwd); 
        strcpy(new_name, new_path); 
    } else if (last_slash == 0) { 
        strcpy(new_parent, "/"); 
        strcpy(new_name, new_path + 1); 
    } else { 
        strncpy(new_parent, new_path, last_slash); 
        new_parent[last_slash] = 0; 
        strcpy(new_name, new_path + last_slash + 1); 
    }
    
    u32 new_dir_cluster = resolve_path(new_parent);
    if (new_dir_cluster == 0) return -1;
    
    /* 4. Perform Rename / Move */
    if (old_dir_cluster == new_dir_cluster) {
        /* Case A: Rename within the same directory */
        /* We only need to update the Name Entry */
        u8* buf = (u8*)kmalloc(CLUSTER_SIZE);
        read_cluster(old_entry_cluster, buf);
        
        /* The Name Entry is at index + 2 */
        exfat_name_entry_t* name_ent = (exfat_name_entry_t*)(buf + (old_entry_index + 2) * 32);
        
        memset(name_ent->name, 0, 30);
        for(int i=0; i<15 && new_name[i]; i++) {
            name_ent->name[i] = new_name[i];
        }
        
        write_cluster(old_entry_cluster, buf);
        kfree(buf);
        return 0;
    } else {
        /* Case B: Move to a different directory */
        /* 1. Allocate new entry set in destination directory */
        u32 new_entry_cluster; 
        int new_entry_index;
        if (!find_free_slot(new_dir_cluster, 3, &new_entry_cluster, &new_entry_index)) {
            return -1; /* No space in new directory */
        }
        
        u8* buf = (u8*)kmalloc(CLUSTER_SIZE);
        
        /* 2. Write new entries (File, Stream, Name) */
        read_cluster(new_entry_cluster, buf);
        
        exfat_file_entry_t* new_f = (exfat_file_entry_t*)(buf + new_entry_index * 32);
        exfat_stream_entry_t* new_s = (exfat_stream_entry_t*)(buf + (new_entry_index + 1) * 32);
        exfat_name_entry_t* new_n = (exfat_name_entry_t*)(buf + (new_entry_index + 2) * 32);
        
        /* Copy File and Stream metadata from old file */
        memcpy(new_f, &f, 32);
        memcpy(new_s, &s, 32);
        
        /* Set new Name */
        memset(new_n, 0, 32);
        new_n->type = EXFAT_ENTRY_NAME;
        for(int i=0; i<15 && new_name[i]; i++) {
            new_n->name[i] = new_name[i];
        }
        
        write_cluster(new_entry_cluster, buf);
        
        /* 3. Mark old entries as deleted */
        /* We do NOT free the cluster chain (s.first_cluster), as it is now owned by the new entry */
        read_cluster(old_entry_cluster, buf);
        
        /* Clear the "In Use" bit (0x80) for File, Stream, and Name entries */
        ((exfat_entry_t*)(buf + old_entry_index * 32))->type &= ~0x80;
        ((exfat_entry_t*)(buf + (old_entry_index + 1) * 32))->type &= ~0x80;
        ((exfat_entry_t*)(buf + (old_entry_index + 2) * 32))->type &= ~0x80;
        
        write_cluster(old_entry_cluster, buf);
        kfree(buf);
        
        return 0;
    }
}

int fs_exists(const char* path) {
    file_info_t info;
    return fs_stat(path, &info) == 0;
}

int fs_chdir(const char* path) {
    u32 cluster = resolve_path(path);
    if (cluster) {
        cwd_cluster = cluster;
        strcpy(cwd, path); /* Store path string too */
        return 0;
    }
    return -1;
}

int fs_getcwd(char* buffer, u32 size) {
    strncpy(buffer, cwd, size);
    return 0;
}

int fs_read_file(const char* path, void* buffer, u32 offset, u32 size) {
    fs_file_t* f = fs_open(path, O_RDONLY);
    if (!f) return -1;
    if (offset > 0) fs_seek(f, offset, SEEK_SET);
    int res = fs_read(f, buffer, size);
    fs_close(f);
    return res;
}

int fs_write_file(const char* path, const void* buffer, u32 offset, u32 size) {
    fs_file_t* f = fs_open(path, O_WRONLY | O_CREAT);
    if (!f) return -1;
    if (offset > 0) fs_seek(f, offset, SEEK_SET);
    int res = fs_write(f, buffer, size);
    fs_close(f);
    return res;
}

void fs_print_tree(const char* path, int level) {
    fs_dir_t* dir = fs_opendir(path);
    if (!dir) return;
    dir_entry_t entry;
    while (fs_readdir(dir, &entry) == 0) {
        for(int i=0; i<level; i++) serial_print("  ");
        serial_print(entry.name);
        if (entry.is_directory) serial_print("/");
        serial_print("\n");
    }
    fs_closedir(dir);
}

int fs_readdir_file(fs_file_t* file, dir_entry_t* entry) {
    if (!file || !entry) return -1;
    if (!file->is_directory) return -1;
    
    u8* buf = (u8*)kmalloc(CLUSTER_SIZE);
    
    while (1) {
        read_cluster(file->current_cluster, buf);
        
        /* Calculate current index in cluster from cluster_offset */
        int index = file->cluster_offset / 32;
        
        while (index < CLUSTER_SIZE / 32) {
            exfat_entry_t* raw = (exfat_entry_t*)(buf + index * 32);
            
            if (raw->type == 0) { /* End of dir */
                kfree(buf);
                return -1; /* EOF */
            }
            
            if (raw->type == EXFAT_ENTRY_FILE) {
                /* Found file */
                if (index + 2 >= CLUSTER_SIZE / 32) {
                    /* Split entry, skip for now */
                    index++;
                    file->cluster_offset += 32;
                    file->current_offset += 32;
                    continue;
                }
                
                exfat_file_entry_t* f = (exfat_file_entry_t*)raw;
                exfat_stream_entry_t* s = (exfat_stream_entry_t*)(buf + (index + 1) * 32);
                exfat_name_entry_t* n = (exfat_name_entry_t*)(buf + (index + 2) * 32);
                
                /* Advance file state past this set */
                index += 3;
                file->cluster_offset += 96;
                file->current_offset += 96;
                
                if (s->type == EXFAT_ENTRY_INFO && n->type == EXFAT_ENTRY_NAME) {
                    memset(entry->name, 0, 256);
                    for(int i=0; i<15; i++) entry->name[i] = (char)n->name[i];
                    entry->is_directory = (f->file_attributes & EXFAT_ATTR_DIRECTORY) ? 1 : 0;
                    entry->size = s->data_length;
                    
                    kfree(buf);
                    return 0; /* Success */
                }
            } else {
                index++;
                file->cluster_offset += 32;
                file->current_offset += 32;
            }
        }
        
        /* Next cluster */
        u32 next = get_next_cluster(file->current_cluster);
        if (next == FAT_EOF || next == 0) {
            kfree(buf);
            return -1; /* EOF */
        }
        file->current_cluster = next;
        file->cluster_offset = 0;
    }
}
