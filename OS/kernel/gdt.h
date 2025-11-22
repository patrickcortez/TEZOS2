#ifndef GDT_H
#define GDT_H

#include "types.h"

/* GDT entry structure */
typedef struct {
    u16 limit_low;
    u16 base_low;
    u8 base_mid;
    u8 access;
    u8 granularity;
    u8 base_high;
} __attribute__((packed)) gdt_entry_t;

/* GDT pointer */
typedef struct {
    u16 limit;
    u64 base;
} __attribute__((packed)) gdt_ptr_t;

/* TSS structure */
typedef struct {
    u32 reserved0;
    u64 rsp0;  /* Kernel stack pointer */
    u64 rsp1;
    u64 rsp2;
    u64 reserved1;
    u64 ist1;
    u64 ist2;
    u64 ist3;
    u64 ist4;
    u64 ist5;
    u64 ist6;
    u64 ist7;
    u64 reserved2;
    u16 reserved3;
    u16 iomap_base;
} __attribute__((packed)) tss_t;

void gdt_init(void);
void tss_set_kernel_stack(u64 stack);

#endif
