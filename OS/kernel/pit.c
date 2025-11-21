#include "io.h"
#include "idt.h"
#include "types.h"

extern void print_str(const char*);

u64 ticks = 0;

void timer_handler() {
    ticks++;
    if (ticks % 18 == 0) {
        /* print_str("Tick\n"); */
    }
    outb(0x20, 0x20); /* EOI */
}

void pit_init(u32 frequency) {
    u32 divisor = 1193180 / frequency;
    
    outb(0x43, 0x36); /* Command port */
    outb(0x40, (u8)(divisor & 0xFF));
    outb(0x40, (u8)((divisor >> 8) & 0xFF));
}
