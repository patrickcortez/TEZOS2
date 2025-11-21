#include "idt.h"
#include "types.h"

struct idt_entry {
    u16 offset_low;
    u16 selector;
    u8 ist;
    u8 type_attr;
    u16 offset_mid;
    u32 offset_high;
    u32 zero;
} __attribute__((packed));

struct idt_ptr {
    u16 limit;
    u64 base;
} __attribute__((packed));

struct idt_entry idt[256];
struct idt_ptr idtp;

extern void load_idt(u64);

void set_idt_gate(u8 num, u64 base, u16 sel, u8 flags) {
    idt[num].offset_low = base & 0xFFFF;
    idt[num].selector = sel;
    idt[num].ist = 0;
    idt[num].type_attr = flags;
    idt[num].offset_mid = (base >> 16) & 0xFFFF;
    idt[num].offset_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].zero = 0;
}

void idt_install() {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (u64)&idt;
    
    /* Clear IDT */
    for (int i=0; i<256; i++) {
        set_idt_gate(i, 0, 0, 0);
    }

    load_idt((u64)&idtp);
}
