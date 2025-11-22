#ifndef VMM_H
#define VMM_H

#include "types.h"

/* Page size */
#define PAGE_SIZE 4096
#define PAGE_ALIGN(addr) ((addr) & ~(PAGE_SIZE - 1))
#define PAGE_ALIGN_UP(addr) (((addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

/* Page table entry flags */
#define PAGE_PRESENT    (1 << 0)
#define PAGE_WRITE      (1 << 1)
#define PAGE_USER       (1 << 2)
#define PAGE_WRITETHROUGH (1 << 3)
#define PAGE_NO_CACHE   (1 << 4)
#define PAGE_ACCESSED   (1 << 5)
#define PAGE_DIRTY      (1 << 6)
#define PAGE_SIZE_BIT   (1 << 7)
#define PAGE_GLOBAL     (1 << 8)
#define PAGE_NO_EXEC    (1ULL << 63)

/* Address space layout */
#define KERNEL_VIRTUAL_BASE 0xFFFF800000000000ULL
#define USER_VIRTUAL_BASE   0x0000000000000000ULL
#define USER_STACK_TOP      0x00007FFFFFFFE000ULL

/* Page table structure */
typedef u64 page_table_entry_t;

typedef struct {
    page_table_entry_t entries[512];
} __attribute__((aligned(PAGE_SIZE))) page_table_t;

/* Virtual memory context (per-process) */
typedef struct {
    page_table_t* pml4;          /* Top-level page table (CR3) */
    u64 cr3_value;               /* Physical address for CR3 */
} vm_context_t;

/* VMM Initialization */
void vmm_init(void);

/* Page table management */
page_table_t* vmm_create_address_space(void);
void vmm_destroy_address_space(page_table_t* pml4);
void vmm_switch_address_space(page_table_t* pml4);

/* Page mapping */
int vmm_map_page(page_table_t* pml4, u64 virt, u64 phys, u64 flags);
int vmm_unmap_page(page_table_t* pml4, u64 virt);
u64 vmm_get_physical_address(page_table_t* pml4, u64 virt);

/* Kernel mapping */
void vmm_map_kernel(page_table_t* pml4);

/* Utility */
void vmm_invalidate_page(u64 virt);

#endif
