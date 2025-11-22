#include "reboot.h"
#include "io.h"
#include "types.h"

/* Reboot the computer - VirtualBox compatible */
void reboot() {
    /* Disable interrupts */
    asm volatile("cli");
    
    /* Method 1: Try ACPI reset (works in VirtualBox) */
    outb(0x64, 0xFE);
    
    /* Small delay */
    for (volatile int i = 0; i < 1000000; i++);
    
    /* Method 2: Triple fault (fallback) */
    /* Load invalid IDT to trigger triple fault */
    struct {
        u16 limit;
        u64 base;
    } __attribute__((packed)) idtr = {0, 0};
    
    asm volatile("lidt %0" : : "m"(idtr));
    asm volatile("int $0x00");  /* Trigger interrupt with invalid IDT */
    
    /* Method 3: Hang if all else fails */
    while (1) {
        asm volatile("hlt");
    }
}
