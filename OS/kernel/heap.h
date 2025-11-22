#ifndef HEAP_H
#define HEAP_H

#include "types.h"

void init_heap(u64 start_addr, u64 size);
void* kmalloc(u64 size);
void kfree(void* ptr);

/* Heap expansion */
int heap_expand(u64 additional_pages);
void heap_shrink(void);

/* Statistics */
u64 heap_get_used(void);
u64 heap_get_free(void);
u64 heap_get_total(void);
void heap_print_stats(void);

#endif
