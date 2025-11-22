#include "elf.h"
#include "task.h"
#include "vmm.h"
#include "pmm.h"
#include "fs.h"
#include "string.h"
#include "serial.h"
#include "heap.h"

int elf_load(const char* path, u64* entry_point) {
    serial_print("[ELF] Loading: ");
    serial_print(path);
    serial_print("\n");
    
    /* Get file info */
    file_info_t info;
    if (fs_stat(path, &info) < 0) {
        serial_print("[ELF] File not found\n");
        return -1;
    }
    
    /* Read ELF file */
    u8* elf_data = (u8*)kmalloc(info.size);
    if (!elf_data) return -1;
    
    u64 bytes_read = fs_read_file(path, elf_data, 0, info.size);
    if (bytes_read != info.size) {
        kfree(elf_data);
        return -1;
    }
    
    /* Parse ELF header */
    elf_header_t* header = (elf_header_t*)elf_data;
    
    /* Verify ELF magic */
    if (header->magic != ELF_MAGIC) {
        serial_print("[ELF] Invalid ELF magic\n");
        kfree(elf_data);
        return -1;
    }
    
    /* Get entry point */
    *entry_point = header->entry;
    
    serial_print("[ELF] Entry point: ");
    serial_print_hex(*entry_point);
    serial_print("\n");
    
    /* Load program segments */
    elf_program_header_t* ph = (elf_program_header_t*)(elf_data + header->phoff);
    
    for (int i = 0; i < header->phnum; i++) {
        if (ph[i].type == PT_LOAD) {
            serial_print("[ELF] Loading segment ");
            serial_print_dec(i);
            serial_print(" at ");
            serial_print_hex(ph[i].vaddr);
            serial_print("\n");
            
            /* Allocate and map pages for this segment */
            u64 vaddr = ph[i].vaddr & ~0xFFF;  /* Page-align */
            u64 pages = (ph[i].memsz + 0xFFF) / 0x1000;
            
            task_t* current = task_get_current();
            for (u64 p = 0; p < pages; p++) {
                void* phys_page = pmm_alloc_page();
                if (!phys_page) {
                    kfree(elf_data);
                    return -1;
                }
                
                /* Map page */
                vmm_map_page(current->page_dir, vaddr + (p * 0x1000),
                            (u64)phys_page, PAGE_WRITE | PAGE_USER);
                
                /* Clear page */
                memset(phys_page, 0, 0x1000);
            }
            
            /* Copy segment data */
            if (ph[i].filesz > 0) {
                memcpy((void*)ph[i].vaddr, elf_data + ph[i].offset, ph[i].filesz);
            }
        }
    }
    
    kfree(elf_data);
    
    serial_print("[ELF] Loaded successfully\n");
    return 0;
}

int elf_exec(const char* path) {
    u64 entry;
    if (elf_load(path, &entry) < 0) {
        return -1;
    }
    
    /* Get current task*/
    task_t* task = task_get_current();
    if (!task) {
        serial_print("[ELF] No current task\n");
        return -1;
    }
    
    /* Set up entry point */
    task->context.rip = entry;
    task->context.rsp = USER_STACK_TOP;
    
    serial_print("[ELF] Exec complete, entry at ");
    serial_print_hex(entry);
    serial_print("\n");
    
    return task->pid;
}
