#include "video.h"
#include "idt.h"
#include "io.h"
#include "multiboot.h"
#include "pmm.h"
#include "serial.h"
#include "fs.h"
#include "task.h"
#include "rootfs.h"
#include "string.h"

extern void irq0_handler();
extern void irq1_handler();
extern void syscall_entry();
extern void keyboard_init();
extern void syscall_init();
extern void pic_remap(int, int);
extern void pit_init(u32);
extern void shell_init();
extern void init_heap(u64, u64);
extern void mbr_init();
extern u32 mbr_get_partition_start(int);
extern void ata_set_partition_offset(u32);
extern void ata_set_partition_offset(u32);
extern void fs_init();
extern int fs_mounted;

extern u64 _end;

/* Global kernel module info */
u64 kernel_module_addr = 0;
u64 kernel_module_size = 0;
u64 multiboot_info_ptr = 0;

void* find_module(const char* cmdline) {
    if (!multiboot_info_ptr) return 0;
    struct multiboot_tag* tag;
    for (tag = (struct multiboot_tag*)(multiboot_info_ptr + 8);
         tag->type != 0;
         tag = (struct multiboot_tag*)((u8*)tag + ((tag->size + 7) & ~7))) {
        if (tag->type == MULTIBOOT_TAG_TYPE_MODULE) {
            struct multiboot_tag_module* mod = (struct multiboot_tag_module*)tag;
            if (strcmp(mod->cmdline, cmdline) == 0) {
                return (void*)(u64)mod->mod_start;
            }
        }
    }
    return 0;
}

void kmain(u64 addr) {
    multiboot_info_ptr = addr;
    u32 mem_upper = 0;
    u64 kernel_end = (u64)&_end;
    init_serial();
    serial_print("\n[KERNEL] Booted!\n");
    
    /* Initialize GDT/TSS early */
    extern void gdt_init(void);
    gdt_init();
    
    /* Initialize Heap (10MB) */
    init_heap((u64)&_end, 1024 * 1024 * 10);
    serial_print("[KERNEL] Heap Initialized at: ");
    serial_print_hex((u64)&_end);
    serial_print("\n");
    
    /* Initialize MBR and mount partition */
    mbr_init();
    u32 partition_start = mbr_get_partition_start(0);
    if (partition_start != 0) {
        ata_set_partition_offset(partition_start);
    }
    
    /* Initialize File System */
    fs_init();
    
    serial_print("[KERNEL] Multiboot Info at: ");
    serial_print_hex(multiboot_info_ptr);
    serial_print("\n");

    /* Debug: Write 'X' to VGA buffer to check if we are in text mode */
    ((u16*)0xB8000)[0] = 0x4F58; // 'X' on Red background

    /* Parse Multiboot Info */
    struct multiboot_tag* tag;
    u64 total_mem = 0;
    int video_initialized = 0;
    
    /* Skip the first 8 bytes (size and reserved) */
    for (tag = (struct multiboot_tag*)(multiboot_info_ptr + 8);
         tag->type != 0;
         tag = (struct multiboot_tag*)((u8*)tag + ((tag->size + 7) & ~7))) {
             
        serial_print("[KERNEL] Tag Type: ");
        serial_print_dec(tag->type);
        serial_print(" Size: ");
        serial_print_dec(tag->size);
        serial_print("\n");

        if (tag->type == MULTIBOOT_TAG_TYPE_BASIC_MEMINFO) {
            struct multiboot_tag_basic_meminfo* meminfo = (struct multiboot_tag_basic_meminfo*)tag;
            mem_upper = meminfo->mem_upper;
            total_mem = (mem_upper + 1024) * 1024;
            pmm_init(total_mem);
            serial_print("[KERNEL] Memory Initialized: ");
            serial_print_dec(total_mem);
            serial_print(" bytes\n");
            
            /* Initialize Virtual Memory Manager */
            extern void vmm_init(void);
            vmm_init();
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
            serial_print("  Type: "); serial_print_dec(fb->common.framebuffer_type); serial_print("\n");
            
            init_video(fb->common.framebuffer_addr, 
                       fb->common.framebuffer_width, 
                       fb->common.framebuffer_height, 
                       fb->common.framebuffer_pitch, 
                       fb->common.framebuffer_bpp,
                       fb->common.framebuffer_type);
            video_initialized = 1;
        }
        else if (tag->type == MULTIBOOT_TAG_TYPE_MODULE) {
            struct multiboot_tag_module* mod = (struct multiboot_tag_module*)tag;
            serial_print("[KERNEL] Module found: ");
            serial_print(mod->cmdline);
            serial_print("\n");
            
            if (strcmp(mod->cmdline, "kernel") == 0) {
                kernel_module_addr = (u64)mod->mod_start;
                kernel_module_size = (u64)(mod->mod_end - mod->mod_start);
                serial_print("Found Kernel Module at: ");
                serial_print_hex(kernel_module_addr);
                serial_print("\n");
            }
        }
    }
    
    /* Initialize PMM with detected memory */
    /* mem_upper is in KB. Convert to bytes. */
    /* We assume at least 128MB for now if not detected */
    if (mem_upper == 0) mem_upper = 128 * 1024;
    
    total_mem = (mem_upper * 1024) + 0x100000; /* +1MB low mem */
    pmm_init(total_mem);
    pmm_free_region(kernel_end, total_mem - kernel_end);
    serial_print("PMM Initialized.\n");

    /* Initialize VMM */
    /* We are already in 64-bit mode with identity mapping for lower memory.
       We will setup a proper kernel page table. */
    extern void vmm_init(void); // Added extern for vmm_init
    vmm_init();
    serial_print("VMM Initialized.\n");
    
    /* Initialize Heap */
    extern void init_heap(u64, u64);
    init_heap(0x2000000, 0x100000); /* 1MB Heap at 32MB */
    serial_print("Heap Initialized.\n");

    /* Initialize Interrupts */
    /* Remap PIC to avoid conflicts (start at 0x20) */
    pic_remap(0x20, 0x28);
    
    /* Register Handlers */
    extern void set_idt_gate(u8, u64, u16, u8); // Added extern for set_idt_gate
    set_idt_gate(32, (u64)irq0_handler, 0x08, 0x8E); /* Timer */
    set_idt_gate(33, (u64)irq1_handler, 0x08, 0x8E); /* Keyboard */
    set_idt_gate(0x80, (u64)syscall_entry, 0x08, 0xEE); /* Syscall */
    
    pit_init(1000); /* 1000 Hz */
    keyboard_init();
    syscall_init();
    
    __asm__ volatile("sti"); /* Enable Interrupts */
    serial_print("Interrupts Enabled.\n");

    /* Initialize Tasking */
    extern void task_init(void); // Added extern for task_init
    task_init();
    serial_print("Tasking Initialized.\n");

    /* Initialize Filesystem */
    /* 1. Initialize MBR/ATA */
    mbr_init();
    
    /* 2. Find Partition */
    u32 part_start = mbr_get_partition_start(0); /* Partition 1 */
    if (part_start > 0) {
        ata_set_partition_offset(part_start);
        serial_print("Partition found at sector: ");
        serial_print_dec(part_start);
        serial_print("\n");
        
        /* 3. Initialize FS */
        fs_init();
    } else {
        serial_print("No partition found. Run 'install' to setup disk.\n");
    }

    /* Verify Drivers on Disk */
    if (fs_mounted) {
        print_str("[KERNEL] Checking drivers in /System/Drivers/...\n");
        
        /* ATA Driver */
        fs_file_t* f = fs_open("/System/Drivers/ata.drv", 0);
        if (f) {
            print_str("  [+] Loading ATA Driver... OK\n");
            fs_close(f);
        } else {
            print_str("  [-] ATA Driver not found on disk! (Using built-in fallback)\n");
        }

        /* VGA Driver */
        f = fs_open("/System/Drivers/vga.drv", 0);
        if (f) {
            print_str("  [+] Loading VGA Driver... OK\n");
            fs_close(f);
        } else {
            print_str("  [-] VGA Driver not found on disk!\n");
        }
        
        /* PS/2 Driver */
        f = fs_open("/System/Drivers/ps2.drv", 0);
        if (f) {
            print_str("  [+] Loading PS/2 Driver... OK\n");
            fs_close(f);
        } else {
            print_str("  [-] PS/2 Driver not found on disk!\n");
        }
    }

    /* Initialize RootFS (Create default files if needed) */
    /* Only if FS is mounted */
    if (fs_mounted) {
        /* Check if /bin/shell exists */

        fs_file_t* f = fs_open("/bin/shell", 0);
        if (!f) {
            /* First boot or re-install needed */
            rootfs_init();
        } else {
            fs_close(f);
        }
    }

    /* Initialize Shell (Userspace) */
    if (fs_mounted) {
        print_str("[KERNEL] Spawning /bin/shell...\n");
        
        /* Create a new task for the shell */
        /* task_create(entry, user_mode) */
        /* We pass NULL as entry because we will load ELF manually */
        task_t* shell_task = task_create(0, 1);
        
        if (shell_task) {
            /* Hack: Temporarily switch CR3 to shell's page dir */
            u64 current_cr3;
            __asm__ volatile("mov %%cr3, %0" : "=r"(current_cr3));
            
            __asm__ volatile("mov %0, %%cr3" :: "r"(shell_task->page_dir));
            
            /* Manually set current task so elf_load uses it */
            extern task_t* current_task;
            task_t* saved_task = current_task;
            current_task = shell_task;
            
            /* Load ELF */
            u64 entry_point;
            extern int elf_load(const char* path, u64* entry);
            if (elf_load("/bin/shell", &entry_point) == 0) {
                shell_task->context.rip = entry_point;
                shell_task->context.rsp = USER_STACK_TOP;
                
                /* Restore CR3 and current task */
                __asm__ volatile("mov %0, %%cr3" :: "r"(current_cr3));
                current_task = saved_task;
                
                /* Add to scheduler (task_create should have done this, but let's ensure) */
                /* Assuming task_create adds to ready queue */
                
                /* Yield to shell */
                print_str("[KERNEL] Yielding to shell...\n");
                schedule();
                
                /* Should not return here immediately if shell runs */
            } else {
                print_str("Failed to load /bin/shell\n");
                __asm__ volatile("mov %0, %%cr3" :: "r"(current_cr3));
                current_task = saved_task;
            }
        }
    } else {
        print_str("File system not mounted. Cannot start shell.\n");
    }

    /* Loop */
    while (1) {
        __asm__ volatile("hlt");
    }
}

