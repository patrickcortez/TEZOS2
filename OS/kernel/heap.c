#include "heap.h"

/* Heap globals */
u64 heap_start_addr = 0;
u64 heap_end_addr = 0;
u64 heap_max_addr = 0;

typedef struct heap_segment_header {
    u64 length;
    struct heap_segment_header* next;
    struct heap_segment_header* prev;
    u8 is_free;
} heap_segment_header_t;

static heap_segment_header_t* first_segment;

void init_heap(u64 start_addr, u64 size) {
    /* Align start address */
    if (start_addr % 8 != 0) {
        u64 adjustment = 8 - (start_addr % 8);
        start_addr += adjustment;
        size -= adjustment;
    }

    first_segment = (heap_segment_header_t*)start_addr;
    first_segment->length = size - sizeof(heap_segment_header_t);
    first_segment->next = 0;
    first_segment->prev = 0;
    first_segment->is_free = 1;
    
    /* Track heap bounds */
    extern u64 heap_start_addr;
    extern u64 heap_end_addr;
    extern u64 heap_max_addr;
    heap_start_addr = start_addr;
    heap_end_addr = start_addr + size;
    heap_max_addr = heap_end_addr;
}

void* kmalloc(u64 size) {
    u64 required_size = size + sizeof(heap_segment_header_t);
    /* Align size */
    if (required_size % 8 != 0) {
        required_size = (required_size + 7) & ~7;
    }

    heap_segment_header_t* current = first_segment;
    while (current) {
        if (current->is_free) {
            if (current->length >= size) {
                /* Found a fit */
                /* Split if enough space */
                if (current->length > required_size + sizeof(heap_segment_header_t) + 8) {
                    u64 split_offset = sizeof(heap_segment_header_t) + size;
                    if (split_offset % 8 != 0) split_offset = (split_offset + 7) & ~7;
                    
                    if (current->length > split_offset + sizeof(heap_segment_header_t)) {
                        heap_segment_header_t* new_segment = (heap_segment_header_t*)((u8*)current + split_offset);
                        new_segment->length = current->length - split_offset;
                        new_segment->next = current->next;
                        new_segment->prev = current;
                        new_segment->is_free = 1;
                        
                        if (new_segment->next) {
                            new_segment->next->prev = new_segment;
                        }
                        
                        current->length = split_offset - sizeof(heap_segment_header_t);
                        current->next = new_segment;
                    }
                }
                
                current->is_free = 0;
                return (void*)((u64)current + sizeof(heap_segment_header_t));
            }
        }
        current = current->next;
    }
    
    /* Out of memory - try to expand heap */
    extern void serial_print(const char*);
    serial_print("[HEAP] Out of memory, expanding...\n");
    
    u64 pages_needed = (required_size + 0xFFF) / 0x1000;
    if (pages_needed < 1) pages_needed = 1;
    
    if (heap_expand(pages_needed) == 0) {
        /* Retry allocation after expansion */
        return kmalloc(size);
    }
    
    return 0; /* Out of memory */
}

void kfree(void* ptr) {
    if (!ptr) return;
    
    heap_segment_header_t* segment = (heap_segment_header_t*)((u64)ptr - sizeof(heap_segment_header_t));
    segment->is_free = 1;
    
    /* Coalesce right */
    if (segment->next && segment->next->is_free) {
        segment->length += sizeof(heap_segment_header_t) + segment->next->length;
        segment->next = segment->next->next;
        if (segment->next) segment->next->prev = segment;
    }
    
    /* Coalesce left */
    if (segment->prev && segment->prev->is_free) {
        segment->prev->length += sizeof(heap_segment_header_t) + segment->length;
        segment->prev->next = segment->next;
        if (segment->next) segment->next->prev = segment->prev;
    }
}

/* Heap expansion and statistics */

int heap_expand(u64 additional_pages) {
    extern void* pmm_alloc_page(void);
    extern void serial_print(const char*);
    extern void serial_print_dec(u64);
    extern void serial_print_hex(u64);
    
    if (!additional_pages) return 0;
    
    for (u64 i = 0; i < additional_pages; i++) {
        void* new_page = pmm_alloc_page();
        if (!new_page) {
            serial_print("[HEAP] Expansion failed, out of memory\n");
            return -1;
        }
        
        /* Extend heap into new page */
        heap_segment_header_t* new_segment = (heap_segment_header_t*)heap_end_addr;
        new_segment->length = 0x1000 - sizeof(heap_segment_header_t);
        new_segment->next = 0;
        new_segment->is_free = 1;
        
        /* Link to previous segment if exists */
        if (first_segment) {
            heap_segment_header_t* current = first_segment;
            while (current->next) current = current->next;
            current->next = new_segment;
            new_segment->prev = current;
        } else {
            first_segment = new_segment;
            new_segment->prev = 0;
        }
        
        heap_end_addr += 0x1000;
    }
    
    serial_print("[HEAP] Expanded by ");
    serial_print_dec(additional_pages);
    serial_print(" pages (");
    serial_print_dec(additional_pages * 0x1000);
    serial_print(" bytes)\n");
    
    return 0;
}

void heap_shrink(void) {
    /* TODO: Implement heap shrinking */
}

u64 heap_get_used(void) {
    u64 used = 0;
    heap_segment_header_t* current = first_segment;
    while (current) {
        if (!current->is_free) {
            used += current->length + sizeof(heap_segment_header_t);
        }
        current = current->next;
    }
    return used;
}

u64 heap_get_free(void) {
    u64 free = 0;
    heap_segment_header_t* current = first_segment;
    while (current) {
        if (current->is_free) {
            free += current->length;
        }
        current = current->next;
    }
    return free;
}

u64 heap_get_total(void) {
    return heap_end_addr - heap_start_addr;
}

void heap_print_stats(void) {
    extern void serial_print(const char*);
    extern void serial_print_dec(u64);
    
    serial_print("[HEAP] Statistics:\n");
    serial_print("  Total: ");
    serial_print_dec(heap_get_total());
    serial_print(" bytes\n  Used: ");
    serial_print_dec(heap_get_used());
    serial_print(" bytes\n  Free: ");
    serial_print_dec(heap_get_free());
    serial_print(" bytes\n");
}
