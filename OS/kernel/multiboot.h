#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include "types.h"

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289

struct multiboot_tag {
    u32 type;
    u32 size;
};

struct multiboot_tag_mmap {
    u32 type;
    u32 size;
    u32 entry_size;
    u32 entry_version;
    struct multiboot_mmap_entry {
        u64 addr;
        u64 len;
        u32 type;
        u32 zero;
    } entries[];
};

struct multiboot_tag_basic_meminfo {
    u32 type;
    u32 size;
    u32 mem_lower;
    u32 mem_upper;
};

#define MULTIBOOT_TAG_TYPE_MMAP 6
#define MULTIBOOT_TAG_TYPE_BASIC_MEMINFO 4
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER 8
#define MULTIBOOT_MEMORY_AVAILABLE 1

struct multiboot_tag_framebuffer_common {
    u32 type;
    u32 size;
    u64 framebuffer_addr;
    u32 framebuffer_pitch;
    u32 framebuffer_width;
    u32 framebuffer_height;
    u8 framebuffer_bpp;
    u8 framebuffer_type;
    u16 reserved;
};

struct multiboot_tag_framebuffer {
    struct multiboot_tag_framebuffer_common common;
};

#endif
