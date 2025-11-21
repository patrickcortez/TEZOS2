#ifndef PMM_H
#define PMM_H

#include "types.h"

void pmm_init(u64 mem_size);
void pmm_free_region(u64 base, u64 length);
void* pmm_alloc_page();
void pmm_free_page(void* ptr);

#endif
