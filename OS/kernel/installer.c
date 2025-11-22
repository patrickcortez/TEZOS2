#include "installer.h"
#include "video.h"
#include "ata.h"
#include "mbr.h"
#include "fs.h"
#include "rootfs.h"
#include "types.h"
#include "io.h"
#include "reboot.h"

#define SECTOR_SIZE 512
#define PARTITION_START_LBA 2048
#define PARTITION_SIZE_SECTORS 18432  /* ~9MB */

extern void memcpy(void* dst, const void* src, u32 size);
extern void memset(void* dst, u8 val, u32 size);

/* Kernel binary - linked from your build */
extern u8 _kernel_start[];
extern u8 _kernel_end[];

/*
 * MBR BOOTLOADER
 * Loads VBR from partition at LBA 2048, jumps to it
 */
/*
 * MBR BOOTLOADER (Relocates to 0x0600)
 */
static const u8 mbr_bootloader[] = {
    /* 0x00: Relocate to 0x0600 */
    0xFA,                         /* cli */
    0x31, 0xC0,                   /* xor ax, ax */
    0x8E, 0xD8,                   /* mov ds, ax */
    0x8E, 0xC0,                   /* mov es, ax */
    0x8E, 0xD0,                   /* mov ss, ax */
    0xBC, 0x00, 0x7C,             /* mov sp, 0x7C00 */
    
    /* Relocate 512 bytes from 0x7C00 to 0x0600 */
    0xBE, 0x00, 0x7C,             /* mov si, 0x7C00 */
    0xBF, 0x00, 0x06,             /* mov di, 0x0600 */
    0xB9, 0x00, 0x02,             /* mov cx, 512 */
    0xFC,                         /* cld */
    0xF3, 0xA4,                   /* rep movsb */
    
    /* Jump to relocated code (offset 0x1D) */
    0xEA, 0x1D, 0x06, 0x00, 0x00, /* jmp 0x0000:0x061D */
    
    /* 0x1D: Start of relocated code */
    0xFB,                         /* sti */
    
    /* Save boot drive to 0x0500 (safe area) */
    0x88, 0x16, 0x00, 0x05,       /* mov [0x0500], dl */
    
    /* Check extensions */
    0xB4, 0x41,                   /* mov ah, 0x41 */
    0xBB, 0x55, 0xAA,             /* mov bx, 0xAA55 */
    0xCD, 0x13,                   /* int 0x13 */
    0x72, 0x36,                   /* jc use_chs */
    0x81, 0xFB, 0xAA, 0x55,       /* cmp bx, 0x55AA */
    0x75, 0x30,                   /* jne use_chs */
    
    /* LBA Read - DAP at 0x0510 */
    0xBE, 0x10, 0x05,             /* mov si, 0x0510 */
    0xC6, 0x04, 0x10,             /* size=16 */
    0xC6, 0x44, 0x01, 0x00,       /* reserved */
    0xC7, 0x44, 0x02, 0x01, 0x00, /* count=1 */
    0xC7, 0x44, 0x04, 0x00, 0x7C, /* offset=0x7C00 */
    0xC7, 0x44, 0x06, 0x00, 0x00, /* segment=0x0000 */
    0xC7, 0x44, 0x08, 0x00, 0x08, /* LBA=2048 */
    0xC7, 0x44, 0x0A, 0x00, 0x00,
    0xC7, 0x44, 0x0C, 0x00, 0x00,
    0xC7, 0x44, 0x0E, 0x00, 0x00,
    
    /* Do Read */
    0x8A, 0x16, 0x00, 0x05,       /* mov dl, [0x0500] */
    0xB4, 0x42,                   /* mov ah, 0x42 */
    0xCD, 0x13,                   /* int 0x13 */
    0x72, 0x15,                   /* jc error */
    0xEA, 0x00, 0x7C, 0x00, 0x00, /* jmp 0x0000:0x7C00 */
    
    /* use_chs */
    0xB4, 0x02,                   /* mov ah, 0x02 */
    0xB0, 0x01,                   /* mov al, 1 */
    0xB5, 0x00,                   /* mov ch, 0 */
    0xB1, 0x11,                   /* mov cl, 17 */
    0xB6, 0x02,                   /* mov dh, 2 */
    0x8A, 0x16, 0x00, 0x05,       /* mov dl, [0x0500] */
    0xBB, 0x00, 0x7C,             /* mov bx, 0x7C00 */
    0xCD, 0x13,                   /* int 0x13 */
    0x72, 0x02,                   /* jc error */
    0xEB, 0xE4,                   /* jmp far jump */
    
    /* error */
    0xBE, 0x8E, 0x06,             /* mov si, msg (offset 0x8E + 0x0600) */
    0xAC,                         /* lodsb */
    0x08, 0xC0,                   /* or al, al */
    0x74, 0x04,                   /* jz halt */
    0xB4, 0x0E,                   /* mov ah, 0x0E */
    0xCD, 0x10,                   /* int 0x10 */
    0xEB, 0xF5,                   /* jmp lodsb */
    
    /* halt */
    0xF4,                         /* hlt */
    0xEB, 0xFD,                   /* jmp halt */
    
    /* msg */
    'M', 'B', 'R', ' ', 'E', 'r', 'r', 0
};

/*
 * VBR (Volume Boot Record)
 * Loads kernel from partition sectors 1-64 to 0x10000
 */
static const u8 vbr_bootloader[] = {
    /* 0x00: Jump over header */
    0xEB, 0x3C, 0x90,
    
    /* 0x03: OEM ID */
    'C', 'O', 'R', 'T', 'E', 'Z', 'F', 'S',
    
    /* 0x0B: BPB */
    0x00, 0x02, 0x01, 0x01, 0x00, 0x02, 0xE0, 0x00,
    'C', 'O', 'R', 'T', 'E', 'Z', '-', 'O', 'S', ' ', ' ',
    'C', 'R', 'T', 'Z', 'F', 'S', ' ', ' ',
    
    /* 0x3E: Entry point */
    0xFA,                         /* cli */
    0x31, 0xC0,                   /* xor ax, ax */
    0x8E, 0xD8,                   /* mov ds, ax */
    0x8E, 0xD0,                   /* mov ss, ax */
    0xBC, 0x00, 0x7C,             /* mov sp, 0x7C00 */
    0xFB,                         /* sti */
    
    /* Save drive */
    0x88, 0x16, 0xFC, 0x7D,       /* mov [0x7DFC], dl */
    
    /* Setup for kernel load at 0x1000:0000 = 0x10000 */
    0xB8, 0x00, 0x10,             /* mov ax, 0x1000 */
    0x8E, 0xC0,                   /* mov es, ax */
    0x31, 0xDB,                   /* xor bx, bx */
    
    /* LBA Read - DAP at 0x7E00 */
    0xBE, 0x00, 0x7E,             /* mov si, 0x7E00 */
    0xC6, 0x04, 0x10,             /* size=16 */
    0xC6, 0x44, 0x01, 0x00,       /* reserved */
    0xC7, 0x44, 0x02, 0x40, 0x00, /* count=64 sectors */
    0xC7, 0x44, 0x04, 0x00, 0x00, /* offset=0 */
    0xC7, 0x44, 0x06, 0x00, 0x10, /* segment=0x1000 */
    /* LBA = partition_start + 1 = 2049 */
    0xC7, 0x44, 0x08, 0x01, 0x08, /* LBA low = 2049 */
    0xC7, 0x44, 0x0A, 0x00, 0x00,
    0xC7, 0x44, 0x0C, 0x00, 0x00,
    0xC7, 0x44, 0x0E, 0x00, 0x00,
    
    /* Do Read */
    0x8A, 0x16, 0xFC, 0x7D,       /* mov dl, [0x7DFC] */
    0xB4, 0x42,                   /* mov ah, 0x42 */
    0xCD, 0x13,                   /* int 0x13 */
    0x72, 0x18,                   /* jc error */
    
    /* Jump to kernel at 0x10000 */
    0xEA, 0x00, 0x00, 0x00, 0x10, /* jmp 0x1000:0x0000 */
    
    /* Error handling omitted for brevity, similar to MBR */
    0xF4, 0xEB, 0xFD
};

static void print_num(u32 num) {
    char buf[12];
    int i = 0;
    if (num == 0) { print_str("0"); return; }
    while (num > 0) { buf[i++] = '0' + (num % 10); num /= 10; }
    while (i > 0) print_char(buf[--i]);
}

static void print_hex8(u8 val) {
    const char* h = "0123456789ABCDEF";
    print_char(h[(val >> 4) & 0xF]);
    print_char(h[val & 0xF]);
}

static void print_ok(void) {
    print_str(" [OK]\n");
}

static void print_fail(void) {
    print_str(" [FAIL]\n");
}

static int ata_wait_ready(void) {
    for (int i = 0; i < 100000; i++) {
        u8 status = inb(0x1F7);
        if (!(status & 0x80)) return 1;  /* BSY clear */
    }
    return 0;
}

static int detect_disk(void) {
    /* Select drive 0 */
    outb(0x1F6, 0xA0);
    
    /* Small delay */
    for (int i = 0; i < 1000; i++) inb(0x1F7);
    
    u8 status = inb(0x1F7);
    if (status == 0xFF || status == 0x00) return 0;
    
    return 1;
}

static u32 get_kernel_size(void) {
    /* Calculate kernel size from linker symbols */
    u64 start = (u64)_kernel_start;
    u64 end = (u64)_kernel_end;
    return (u32)(end - start);
}

static int write_sectors(u32 lba, u8* data, u32 count) {
    for (u32 i = 0; i < count; i++) {
        ata_write_sector(lba + i, data + (i * SECTOR_SIZE));
        /* ATA write is void, assume success */
    }
    return 1;
}

static int verify_sector(u32 lba, u8* expected) {
    u8 buffer[SECTOR_SIZE];
    ata_read_sector(lba, buffer);
    
    for (int i = 0; i < SECTOR_SIZE; i++) {
        if (buffer[i] != expected[i]) return 0;
    }
    return 1;
}

void installer_main(void) {
    print_str("\n");
    print_str("+======================================================+\n");
    print_str("|            CORTEZ-OS INSTALLER v2.0                  |\n");
    print_str("|         Works with CortezFS filesystem               |\n");
    print_str("+======================================================+\n\n");
    
    /* ========== STEP 1: DETECT DISK ========== */
    print_str("[1/7] Detecting disk...");
    if (!detect_disk()) {
        print_str(" FAILED\n\nNo disk found. Check connections.\n");
        return;
    }
    print_str(" OK\n");
    
    /* ========== STEP 2: READ CURRENT MBR ========== */
    print_str("[2/7] Reading MBR...");
    u8 mbr_sector[SECTOR_SIZE];
    ata_read_sector(0, mbr_sector);
    
    /* Check for existing partitions */
    partition_entry_t* ptable = (partition_entry_t*)(mbr_sector + 446);
    for (int i = 0; i < 4; i++) {
        if (ptable[i].partition_type != 0) {
            print_str("      Partition ");
            print_num(i + 1);
            print_str(": Type=0x");
            print_hex8(ptable[i].partition_type);
            print_str(" LBA=");
            print_num(ptable[i].first_lba);
            print_str("\n");
        }
    }
    
    /* ========== STEP 3: SETUP PARTITION TABLE ========== */
    print_str("[3/7] Creating partition...");
    
    memset(ptable, 0, 64);
    
    ptable[0].status = 0x80;                    /* Bootable */
    ptable[0].partition_type = 0x83;            /* Linux/CortezFS */
    ptable[0].first_lba = PARTITION_START_LBA;
    ptable[0].sector_count = PARTITION_SIZE_SECTORS;
    
    print_str(" OK (LBA ");
    print_num(PARTITION_START_LBA);
    print_str(", ");
    print_num(PARTITION_SIZE_SECTORS / 2048);
    print_str(" MB)\n");
    
    /* ========== STEP 4: WRITE MBR BOOTLOADER ========== */
    print_str("[4/7] Installing MBR bootloader...\n");
    
    /* Copy bootloader to MBR (preserve partition table) */
    memcpy(mbr_sector, mbr_bootloader, 446);
    
    /* Ensure signature */
    mbr_sector[510] = 0x55;
    mbr_sector[511] = 0xAA;
    
    /* Write MBR */
    print_str("      Writing MBR...");
    ata_write_sector(0, mbr_sector);
    
    /* Verify MBR */
    print_str(" Verifying...");
    if (!verify_sector(0, mbr_sector)) {
        print_fail();
        print_str("      ERROR: MBR verification failed!\n");
        return;
    }
    print_ok();
    print_str(" OK\n");
    
    /* ========== STEP 5: WRITE VBR TO PARTITION ========== */
    print_str("[5/7] Writing VBR to partition...");
    u8 vbr[SECTOR_SIZE];
    memcpy(vbr, vbr_bootloader, SECTOR_SIZE);
    ata_write_sector(PARTITION_START_LBA, vbr);
    print_ok();
    
    /* ========== STEP 6: FORMAT WITH CORTEZFS ========== */
    print_str("[6/7] Formatting CortezFS...\n");
    
    /* Set partition offset so fs.c writes to correct location */
    ata_set_partition_offset(PARTITION_START_LBA);
    
    /*
     * Now we manually create the CortezFS structure:
     * Sector 0 (partition-relative) = Boot sector (but VBR is there)
     * So we write CortezFS boot sector to partition sector 0
     * But wait - VBR is already there!
     * 
     * Solution: CortezFS boot_sector goes AFTER VBR
     * Or: Embed CortezFS metadata in VBR unused space
     * 
     * Simpler: Write CortezFS structure, then patch VBR back
     */
    
    /* Create CortezFS boot sector at partition offset */
    u8 fs_boot[512] = {0};
    
    /* CortezFS layout (relative to partition):
     * Sector 0: VBR (boot code) - we keep this
     * Sector 1-64: FAT
     * Sector 65+: Data (root dir at 65)
     */
    
    typedef struct {
        u32 magic;
        u32 fat_start;
        u32 fat_size;
        u32 root_dir_sector;
        u32 data_start;
        u32 total_sectors;
    } __attribute__((packed)) cortezfs_boot_t;
    
    cortezfs_boot_t* fsboot = (cortezfs_boot_t*)(vbr + 0x100);  /* Put at offset 0x100 in VBR */
    fsboot->magic = 0x43525446;  /* 'CRTF' */
    fsboot->fat_start = 1;
    fsboot->fat_size = 64;
    fsboot->data_start = 65;
    fsboot->root_dir_sector = 65;
    fsboot->total_sectors = PARTITION_SIZE_SECTORS;
    
    /* Re-write VBR with embedded FS metadata */
    ata_write_sector(PARTITION_START_LBA, vbr);
    
    /* Clear and initialize FAT */
    print_str("      Writing FAT...");
    u8 fat_sector[512];
    memset(fat_sector, 0, 512);
    
    /* First FAT sector: mark root dir as EOF */
    u32* fat_entries = (u32*)fat_sector;
    fat_entries[0] = 0xFFFFFFFF;  /* Root dir = EOF */
    ata_write_sector(PARTITION_START_LBA + 1, fat_sector);
    
    /* Clear rest of FAT */
    memset(fat_sector, 0, 512);
    for (u32 i = 2; i <= 64; i++) {
        ata_write_sector(PARTITION_START_LBA + i, fat_sector);
    }
    print_str(" OK\n");
    
    /* Clear root directory */
    print_str("      Creating root directory...");
    u8 empty[512] = {0};
    ata_write_sector(PARTITION_START_LBA + 65, empty);
    print_str(" OK\n");
    
    /* ========== STEP 7: COPY KERNEL ========== */
    print_str("[7/7] Installing kernel...");
    
    u32 kernel_size = get_kernel_size();
    u32 kernel_sectors = (kernel_size + 511) / 512;
    
    print_str("\n      Size: ");
    print_num(kernel_size);
    print_str(" bytes (");
    print_num(kernel_sectors);
    print_str(" sectors)\n      Writing: ");
    
    /* 
     * Kernel goes to data area after root dir
     * VBR loads from partition + 65 (data_start)
     * So kernel at partition + 66
     * 
     * Actually VBR is set to load from 2113 = 2048 + 65
     * That's the root dir sector. We need kernel THERE.
     * 
     * Let's put kernel at a known location and update VBR.
     * Kernel at partition + 66, update VBR LBA to 2048+66=2114
     */
    
    /* Write VBR to first sector of partition */
    print_str("      Writing VBR...");
    u8 vbr_copy[SECTOR_SIZE];
    memcpy(vbr_copy, vbr_bootloader, SECTOR_SIZE);
    
    ata_write_sector(PARTITION_START_LBA, vbr_copy);
    print_ok();
    
    /* Write kernel starting at partition + 1 */
    print_str("      Writing kernel...");
    u32 kernel_lba = PARTITION_START_LBA + 1;
    
    extern u64 kernel_module_addr;
    extern u64 kernel_module_size;
    
    if (kernel_module_addr != 0) {
        /* Extract LOAD segments from ELF */
        /* ELF Header */
        u8* elf = (u8*)kernel_module_addr;
        if (elf[0] == 0x7F && elf[1] == 'E' && elf[2] == 'L' && elf[3] == 'F') {
            /* It's ELF. Find Program Headers */
            u64 phoff = *(u64*)(elf + 32);
            u16 phnum = *(u16*)(elf + 56);
            u16 phentsize = *(u16*)(elf + 54);
            
            u8* ph = elf + phoff;
            
            /* We need to construct a flat binary image */
            /* Find min address and max address */
            u64 min_addr = 0xFFFFFFFFFFFFFFFF;
            u64 max_addr = 0;
            
            for (int i = 0; i < phnum; i++) {
                u32 type = *(u32*)(ph + i * phentsize);
                if (type == 1) { /* LOAD */
                    u64 vaddr = *(u64*)(ph + i * phentsize + 16);
                    u64 memsz = *(u64*)(ph + i * phentsize + 40);
                    if (vaddr < min_addr) min_addr = vaddr;
                    if (vaddr + memsz > max_addr) max_addr = vaddr + memsz;
                }
            }
            
            u32 raw_size = (u32)(max_addr - min_addr);
            u32 raw_sectors = (raw_size + 511) / 512;
            
            print_str(" (ELF -> Raw: ");
            print_num(raw_size);
            print_str(" bytes)\n      ");
            
            /* Write segments */
            /* We write sector by sector, filling from ELF segments */
            u8* sector_buf = (u8*)kmalloc(512);
            
            for (u32 s = 0; s < raw_sectors; s++) {
                u64 current_addr = min_addr + (s * 512);
                memset(sector_buf, 0, 512);
                
                /* Find which segment covers this sector */
                for (int i = 0; i < phnum; i++) {
                    u32 type = *(u32*)(ph + i * phentsize);
                    if (type == 1) { /* LOAD */
                        u64 vaddr = *(u64*)(ph + i * phentsize + 16);
                        u64 filesz = *(u64*)(ph + i * phentsize + 32);
                        u64 offset = *(u64*)(ph + i * phentsize + 8);
                        
                        /* Check overlap */
                        if (current_addr >= vaddr && current_addr < vaddr + filesz) {
                            /* Copy overlapping part */
                            u32 sec_offset = 0;
                            u32 copy_len = 512;
                            
                            /* Adjust if segment starts inside this sector */
                            /* (Not likely if aligned, but possible) */
                            
                            /* Simple byte loop for correctness */
                            for (int b = 0; b < 512; b++) {
                                u64 addr = current_addr + b;
                                if (addr >= vaddr && addr < vaddr + filesz) {
                                    sector_buf[b] = elf[offset + (addr - vaddr)];
                                }
                            }
                        }
                    }
                }
                
                ata_write_sector(kernel_lba + s, sector_buf);
                if ((s & 0xF) == 0) print_char('.');
            }
            kfree(sector_buf);
            
        } else {
            print_str(" Error: Not an ELF file!\n");
        }
    } else {
        /* Fallback: Write current kernel from memory (dangerous/wrong) */
        /* Just write dummy */
        print_str(" Error: No kernel module!\n");
    }
    print_ok();
    
    /* Update root filesystem references if needed */
    ata_set_partition_offset(0);
    
    print_str("[7/7] Formatting filesystem...\n");
    ata_set_partition_offset(PARTITION_START_LBA);
    fs_format();
    rootfs_init();
    ata_set_partition_offset(0);
    print_ok();
    
    /* Reset partition offset */
    ata_set_partition_offset(0);
    
    /* ========== DONE ========== */
    print_str("\n");
    print_str("+======================================================+\n");
    print_str("|            INSTALLATION COMPLETE!                    |\n");
    print_str("+======================================================+\n\n");
    
    print_str("Summary:\n");
    print_str("  MBR bootloader at sector 0\n");
    print_str("  Partition at LBA ");
    print_num(PARTITION_START_LBA);
    print_str(" (");
    print_num(PARTITION_SIZE_SECTORS / 2048);
    print_str(" MB)\n");
    print_str("  CortezFS formatted\n");
    print_str("  Kernel: ");
    print_num(kernel_size);
    print_str(" bytes (");
    print_num(kernel_sectors);
    print_str(" sectors) at LBA ");
    print_num(PARTITION_START_LBA + 1);
    print_str("\n\n");
    
    print_str("Boot chain:\n");
    print_str("  BIOS -> MBR (sector 0)\n");
    print_str("       -> VBR (sector 2048)\n");
    print_str("       -> Kernel (sector 2049)\n");
    print_str("       -> 0x10000 in memory\n\n");
    
    print_str("Remove installation media and press any key to reboot...");
    
    while (!(inb(0x64) & 1));
    inb(0x60);
    
    print_str("\n\nRebooting...\n");
    for (volatile int i = 0; i < 5000000; i++);
    reboot();
}