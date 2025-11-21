#include "pmm.h"
#include "video.h"

/* Bitmap to track physical pages (4KB each) */
/* 128MB covered for now (32768 pages) */
#define PMM_BITMAP_SIZE 32768 / 8 
static u8 pmm_bitmap[PMM_BITMAP_SIZE];
static u64 total_memory = 0;
static u64 used_memory = 0;

extern void print_str(const char* str);
extern void print_char(char c);

/* Helper to print hex (for debugging) */
void print_hex(u64 n) {
    char hex_chars[] = "0123456789ABCDEF";
    print_str("0x");
    for (int i = 60; i >= 0; i -= 4) {
        print_char(hex_chars[(n >> i) & 0xF]);
    }
}

void pmm_init(u64 mem_size) {
    total_memory = mem_size;
    used_memory = 0;
    
    /* Mark all as used initially */
    for (int i = 0; i < PMM_BITMAP_SIZE; i++) {
        pmm_bitmap[i] = 0xFF;
    }
    
    print_str("PMM Initialized. Total Memory: ");
    print_hex(total_memory);
    print_str("\n");
}

void pmm_free_region(u64 base, u64 length) {
    u64 page_start = base / 4096;
    u64 pages = length / 4096;
    
    for (u64 i = 0; i < pages; i++) {
        u64 page = page_start + i;
        if (page / 8 >= PMM_BITMAP_SIZE) break;
        
        pmm_bitmap[page / 8] &= ~(1 << (page % 8));
        used_memory -= 4096;
    }
}

void* pmm_alloc_page() {
    for (int i = 0; i < PMM_BITMAP_SIZE; i++) {
        if (pmm_bitmap[i] != 0xFF) {
            for (int j = 0; j < 8; j++) {
                if (!(pmm_bitmap[i] & (1 << j))) {
                    pmm_bitmap[i] |= (1 << j);
                    used_memory += 4096;
                    return (void*)((u64)(i * 8 + j) * 4096);
                }
            }
        }
    }
    return 0; /* Out of memory */
}

void pmm_free_page(void* ptr) {
    u64 page = (u64)ptr / 4096;
    if (page / 8 < PMM_BITMAP_SIZE) {
        pmm_bitmap[page / 8] &= ~(1 << (page % 8));
        used_memory -= 4096;
    }
}
