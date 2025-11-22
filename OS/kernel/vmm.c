#include "vmm.h"
#include "pmm.h"
#include "string.h"
#include "serial.h"

/* Current kernel page tables */
static page_table_t* kernel_pml4 = 0;

/* Extract page table indices from virtual address */
#define PML4_INDEX(addr) (((addr) >> 39) & 0x1FF)
#define PDPT_INDEX(addr) (((addr) >> 30) & 0x1FF)
#define PD_INDEX(addr)   (((addr) >> 21) & 0x1FF)
#define PT_INDEX(addr)   (((addr) >> 12) & 0x1FF)

/* Get physical address from entry */
#define PTE_ADDR(entry) ((entry) & 0x000FFFFFFFFFF000ULL)

/* Allocate a page table */
static page_table_t* alloc_page_table(void) {
    void* page = pmm_alloc_page();
    if (!page) return 0;
    
    /* Zero out the page table */
    memset(page, 0, PAGE_SIZE);
    return (page_table_t*)page;
}

/* Get or create page table at given entry */
static page_table_t* get_or_create_table(page_table_entry_t* entry, u64 flags) {
    if (*entry & PAGE_PRESENT) {
        /* Table already exists */
        return (page_table_t*)PTE_ADDR(*entry);
    }
    
    /* Allocate new table */
    page_table_t* table = alloc_page_table();
    if (!table) return 0;
    
    /* Set entry */
    *entry = (u64)table | (flags & 0xFFF) | PAGE_PRESENT | PAGE_WRITE;
    return table;
}

void vmm_init(void) {
    serial_print("[VMM] Initializing virtual memory manager...\n");
    
    /* Create kernel page tables */
    kernel_pml4 = alloc_page_table();
    if (!kernel_pml4) {
        serial_print("[VMM] FATAL: Could not allocate kernel PML4!\n");
        return;
    }
    
    serial_print("[VMM] Kernel PML4 at: ");
    serial_print_hex((u64)kernel_pml4);
    serial_print("\n");
    
    /* Identity map first 4GB for kernel (0x0 - 0xFFFFFFFF) */
    /* This allows kernel to access hardware memory directly */
    serial_print("[VMM] Identity mapping first 4GB...\n");
    for (u64 addr = 0; addr < 0x100000000ULL; addr += 0x200000) {
        /* Use 2MB pages for efficiency */
        u64 pml4_idx = PML4_INDEX(addr);
        u64 pdpt_idx = PDPT_INDEX(addr);
        u64 pd_idx = PD_INDEX(addr);
        
        /* Get/create PDPT */
        page_table_t* pdpt = get_or_create_table(
            &kernel_pml4->entries[pml4_idx], 
            PAGE_WRITE | PAGE_USER
        );
        
        /* Get/create PD */
        page_table_t* pd = get_or_create_table(
            &pdpt->entries[pdpt_idx],
            PAGE_WRITE | PAGE_USER
        );
        
        /* Map 2MB page */
        pd->entries[pd_idx] = addr | PAGE_PRESENT | PAGE_WRITE | 
                              PAGE_SIZE_BIT | PAGE_GLOBAL;
    }
    
    /* Also map kernel to higher half (0xFFFF800000000000 + phys) */
    serial_print("[VMM] Mapping kernel to higher half...\n");
    for (u64 phys = 0; phys < 0x10000000ULL; phys += 0x200000) {
        u64 virt = KERNEL_VIRTUAL_BASE + phys;
        
        u64 pml4_idx = PML4_INDEX(virt);
        u64 pdpt_idx = PDPT_INDEX(virt);
        u64 pd_idx = PD_INDEX(virt);
        
        page_table_t* pdpt = get_or_create_table(
            &kernel_pml4->entries[pml4_idx],
            PAGE_WRITE
        );
        
        page_table_t* pd = get_or_create_table(
            &pdpt->entries[pdpt_idx],
            PAGE_WRITE
        );
        
        pd->entries[pd_idx] = phys | PAGE_PRESENT | PAGE_WRITE |
                              PAGE_SIZE_BIT | PAGE_GLOBAL;
    }
    
    /* Switch to new page tables */
    serial_print("[VMM] Switching to kernel page tables...\n");
    __asm__ volatile("mov %0, %%cr3" :: "r"(kernel_pml4));
    
    /* Enable global pages */
    u64 cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 7);  /* PGE bit */
    __asm__ volatile("mov %0, %%cr4" :: "r"(cr4));
    
    serial_print("[VMM] Virtual memory initialized!\n");
}

page_table_t* vmm_create_address_space(void) {
    page_table_t* pml4 = alloc_page_table();
    if (!pml4) return 0;
    
    /* Copy kernel mappings to new address space */
    /* Upper half (kernel space) should be shared */
    for (int i = 256; i < 512; i++) {
        pml4->entries[i] = kernel_pml4->entries[i];
    }
    
    return pml4;
}

void vmm_destroy_address_space(page_table_t* pml4) {
    if (!pml4 || pml4 == kernel_pml4) return;
    
    /* Free user space page tables (lower half) */
    for (int i = 0; i < 256; i++) {
        if (!(pml4->entries[i] & PAGE_PRESENT)) continue;
        
        page_table_t* pdpt = (page_table_t*)PTE_ADDR(pml4->entries[i]);
        
        for (int j = 0; j < 512; j++) {
            if (!(pdpt->entries[j] & PAGE_PRESENT)) continue;
            
            page_table_t* pd = (page_table_t*)PTE_ADDR(pdpt->entries[j]);
            
            for (int k = 0; k < 512; k++) {
                if (!(pd->entries[k] & PAGE_PRESENT)) continue;
                if (pd->entries[k] & PAGE_SIZE_BIT) continue;  /* 2MB page */
                
                page_table_t* pt = (page_table_t*)PTE_ADDR(pd->entries[k]);
                pmm_free_page(pt);
            }
            
            pmm_free_page(pd);
        }
        
        pmm_free_page(pdpt);
    }
    
    pmm_free_page(pml4);
}

void vmm_switch_address_space(page_table_t* pml4) {
    if (!pml4) pml4 = kernel_pml4;
    __asm__ volatile("mov %0, %%cr3" :: "r"(pml4));
}

int vmm_map_page(page_table_t* pml4, u64 virt, u64 phys, u64 flags) {
    if (!pml4) pml4 = kernel_pml4;
    
    u64 pml4_idx = PML4_INDEX(virt);
    u64 pdpt_idx = PDPT_INDEX(virt);
    u64 pd_idx = PD_INDEX(virt);
    u64 pt_idx = PT_INDEX(virt);
    
    /* Get/create tables */
    page_table_t* pdpt = get_or_create_table(&pml4->entries[pml4_idx], flags);
    if (!pdpt) return -1;
    
    page_table_t* pd = get_or_create_table(&pdpt->entries[pdpt_idx], flags);
    if (!pd) return -1;
    
    page_table_t* pt = get_or_create_table(&pd->entries[pd_idx], flags);
    if (!pt) return -1;
    
    /* Set page entry */
    pt->entries[pt_idx] = (phys & ~0xFFFULL) | (flags & 0xFFF) | PAGE_PRESENT;
    
    /* Invalidate TLB for this page */
    vmm_invalidate_page(virt);
    
    return 0;
}

int vmm_unmap_page(page_table_t* pml4, u64 virt) {
    if (!pml4) pml4 = kernel_pml4;
    
    u64 pml4_idx = PML4_INDEX(virt);
    u64 pdpt_idx = PDPT_INDEX(virt);
    u64 pd_idx = PD_INDEX(virt);
    u64 pt_idx = PT_INDEX(virt);
    
    /* Navigate to page table */
    if (!(pml4->entries[pml4_idx] & PAGE_PRESENT)) return -1;
    page_table_t* pdpt = (page_table_t*)PTE_ADDR(pml4->entries[pml4_idx]);
    
    if (!(pdpt->entries[pdpt_idx] & PAGE_PRESENT)) return -1;
    page_table_t* pd = (page_table_t*)PTE_ADDR(pdpt->entries[pdpt_idx]);
    
    if (!(pd->entries[pd_idx] & PAGE_PRESENT)) return -1;
    page_table_t* pt = (page_table_t*)PTE_ADDR(pd->entries[pd_idx]);
    
    /* Clear entry */
    pt->entries[pt_idx] = 0;
    
    /* Invalidate TLB */
    vmm_invalidate_page(virt);
    
    return 0;
}

u64 vmm_get_physical_address(page_table_t* pml4, u64 virt) {
    if (!pml4) pml4 = kernel_pml4;
    
    u64 pml4_idx = PML4_INDEX(virt);
    u64 pdpt_idx = PDPT_INDEX(virt);
    u64 pd_idx = PD_INDEX(virt);
    u64 pt_idx = PT_INDEX(virt);
    
    if (!(pml4->entries[pml4_idx] & PAGE_PRESENT)) return 0;
    page_table_t* pdpt = (page_table_t*)PTE_ADDR(pml4->entries[pml4_idx]);
    
    if (!(pdpt->entries[pdpt_idx] & PAGE_PRESENT)) return 0;
    page_table_t* pd = (page_table_t*)PTE_ADDR(pdpt->entries[pdpt_idx]);
    
    if (!(pd->entries[pd_idx] & PAGE_PRESENT)) return 0;
    
    /* Check for 2MB page */
    if (pd->entries[pd_idx] & PAGE_SIZE_BIT) {
        return PTE_ADDR(pd->entries[pd_idx]) + (virt & 0x1FFFFF);
    }
    
    page_table_t* pt = (page_table_t*)PTE_ADDR(pd->entries[pd_idx]);
    
    if (!(pt->entries[pt_idx] & PAGE_PRESENT)) return 0;
    
    return PTE_ADDR(pt->entries[pt_idx]) | (virt & 0xFFF);
}

void vmm_invalidate_page(u64 virt) {
    __asm__ volatile("invlpg (%0)" :: "r"(virt) : "memory");
}

void vmm_map_kernel(page_table_t* pml4) {
    /* Copy kernel mappings from kernel_pml4 */
    for (int i = 256; i < 512; i++) {
        pml4->entries[i] = kernel_pml4->entries[i];
    }
}

