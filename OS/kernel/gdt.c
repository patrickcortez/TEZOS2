#include "gdt.h"
#include "serial.h"
#include "string.h"

/* GDT entries */
static gdt_entry_t gdt[7];
static gdt_ptr_t gdt_ptr;
static tss_t tss;

extern void gdt_flush(u64 gdt_ptr);  /* Assembly function */
extern void tss_flush(void);          /* Assembly function */

static void gdt_set_gate(int num, u64 base, u32 limit, u8 access, u8 gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_mid = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;
    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access = access;
}

void gdt_init(void) {
    serial_print("[GDT] Initializing Global Descriptor Table...\n");
    
    /* Setup GDT pointer */
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 7) - 1;
    gdt_ptr.base = (u64)&gdt;
    
    /* Null descriptor */
    gdt_set_gate(0, 0, 0, 0, 0);
    
    /* Kernel code segment (0x08) */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xA0);
    
    /* Kernel data segment (0x10) */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xA0);
    
    /* User code segment (0x18) - RPL=3 */
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xA0);
    
    /* User data segment (0x20) - RPL=3 */
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xA0);
    
    /* TSS (Task State Segment) - 0x28 */
    u64 tss_base = (u64)&tss;
    u32 tss_limit = sizeof(tss_t);
    
    memset(&tss, 0, sizeof(tss_t));
    tss.iomap_base = sizeof(tss_t);
    
    /* TSS descriptor (takes 2 entries in 64-bit mode) */
    gdt[5].limit_low = tss_limit & 0xFFFF;
    gdt[5].base_low = tss_base & 0xFFFF;
    gdt[5].base_mid = (tss_base >> 16) & 0xFF;
    gdt[5].access = 0x89;  /* Present, Ring 0, TSS */
    gdt[5].granularity = 0x00;
    gdt[5].base_high = (tss_base >> 24) & 0xFF;
    
    /* High part of TSS base (64-bit) */
    gdt[6].limit_low = (tss_base >> 32) & 0xFFFF;
    gdt[6].base_low = (tss_base >> 48) & 0xFFFF;
    gdt[6].base_mid = 0;
    gdt[6].access = 0;
    gdt[6].granularity = 0;
    gdt[6].base_high = 0;
    
    /* Load GDT */
    gdt_flush((u64)&gdt_ptr);
    
    /* Load TSS */
    tss_flush();
    
    serial_print("[GDT] GDT and TSS loaded\n");
}

void tss_set_kernel_stack(u64 stack) {
    tss.rsp0 = stack;
}
