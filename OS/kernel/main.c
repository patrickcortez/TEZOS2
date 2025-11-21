#include "video.h"
#include "idt.h"
#include "io.h"
#include "multiboot.h"
#include "pmm.h"
#include "serial.h"

extern void irq0_handler();
extern void irq1_handler();
extern void pic_remap(int, int);
extern void pit_init(u32);

void kmain(u64 multiboot_addr) {
    init_serial();
    serial_print("\n[KERNEL] Booted!\n");
    serial_print("[KERNEL] Multiboot Info at: ");
    serial_print_hex(multiboot_addr);
    serial_print("\n");

    /* Debug: Write 'X' to VGA buffer to check if we are in text mode */
    ((u16*)0xB8000)[0] = 0x4F58; // 'X' on Red background

    /* Parse Multiboot Info */
    struct multiboot_tag* tag;
    u64 total_mem = 0;
    
    /* Skip the first 8 bytes (size and reserved) */
    for (tag = (struct multiboot_tag*)(multiboot_addr + 8);
         tag->type != 0;
         tag = (struct multiboot_tag*)((u8*)tag + ((tag->size + 7) & ~7))) {
             
        serial_print("[KERNEL] Tag Type: ");
        serial_print_dec(tag->type);
        serial_print(" Size: ");
        serial_print_dec(tag->size);
        serial_print("\n");

        if (tag->type == MULTIBOOT_TAG_TYPE_BASIC_MEMINFO) {
            struct multiboot_tag_basic_meminfo* meminfo = (struct multiboot_tag_basic_meminfo*)tag;
            total_mem = (meminfo->mem_upper + 1024) * 1024;
            pmm_init(total_mem);
            serial_print("[KERNEL] Memory Initialized: ");
            serial_print_dec(total_mem);
            serial_print(" bytes\n");
        }
        else if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
            struct multiboot_tag_mmap* mmap = (struct multiboot_tag_mmap*)tag;
            struct multiboot_mmap_entry* entry;
            
            for (entry = mmap->entries;
                 (u8*)entry < (u8*)mmap + mmap->size;
                 entry = (struct multiboot_mmap_entry*)((u64)entry + mmap->entry_size)) {
                
                if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
                    pmm_free_region(entry->addr, entry->len);
                }
            }
        }
        else if (tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
            struct multiboot_tag_framebuffer* fb = (struct multiboot_tag_framebuffer*)tag;
            serial_print("[KERNEL] Framebuffer Found!\n");
            serial_print("  Addr: "); serial_print_hex(fb->common.framebuffer_addr); serial_print("\n");
            serial_print("  Width: "); serial_print_dec(fb->common.framebuffer_width); serial_print("\n");
            serial_print("  Height: "); serial_print_dec(fb->common.framebuffer_height); serial_print("\n");
            serial_print("  Pitch: "); serial_print_dec(fb->common.framebuffer_pitch); serial_print("\n");
            serial_print("  BPP: "); serial_print_dec(fb->common.framebuffer_bpp); serial_print("\n");

            init_video(fb->common.framebuffer_addr, 
                       fb->common.framebuffer_width, 
                       fb->common.framebuffer_height, 
                       fb->common.framebuffer_pitch, 
                       fb->common.framebuffer_bpp);
        }
    }

    print_str("Welcome to Cortez-OS!\n");
    print_str("Initializing Interrupts...\n");

    /* Remap PIC to avoid conflicts (start at 0x20) */
    pic_remap(0x20, 0x28);
    
    /* Install IDT */
    idt_install();
    
    /* Register Timer Handler (IRQ0 -> 0x20) */
    set_idt_gate(0x20, (u64)irq0_handler, 0x08, 0x8E);
    pit_init(50); /* 50 Hz */
    
    /* Register Keyboard Handler (IRQ1 -> 0x21) */
    /* 0x8E = Present, Ring0, Interrupt Gate */
    set_idt_gate(0x21, (u64)irq1_handler, 0x08, 0x8E);
    
    /* Enable Interrupts */
    __asm__ volatile ("sti");
    
    print_str("System initialized. Type 'help' for commands.\n");
    print_str("> ");
    
    /* Halt loop */
    while(1) {
        __asm__("hlt");
    }
}
