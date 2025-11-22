#include "io.h"
#include "idt.h"
#include "types.h"

extern void print_str(const char*);

static u32 ticks = 0; // Changed type to u32 and added static

void timer_handler(void) { // Changed signature to void
    ticks++;
    
    /* Call scheduler tick */
    extern void scheduler_tick(void);
    scheduler_tick();

    outb(0x20, 0x20); /* EOI */ // Kept EOI
}

void pit_init(u32 hz) { // Changed parameter name to hz
    u32 divisor = 1193180 / hz; // Changed frequency to hz
    
    outb(0x43, 0x36); // Removed comment
    outb(0x40, divisor & 0xFF); // Simplified cast
    outb(0x40, (divisor >> 8) & 0xFF); // Simplified cast
}
