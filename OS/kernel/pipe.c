#include "pipe.h"
#include "string.h"
#include "pmm.h"
#include "serial.h"
#include "task.h"

int pipe_create(int fds[2]) {
    /* Allocate pipe structure */
    pipe_t* pipe = (pipe_t*)pmm_alloc_page();
    if (!pipe) return -1;
    
    memset(pipe, 0, sizeof(pipe_t));
    
    /* Assign file descriptors */
    /* Find free FDs in current task */
    task_t* current = task_get_current();
    if (!current) {
        pmm_free_page(pipe);
        return -1;
    }
    
    int read_fd = -1, write_fd = -1;
    
    /* Find two free FDs */
    for (int i = 3; i < MAX_FDS && (read_fd < 0 || write_fd < 0); i++) {
        if (!current->fds[i]) {
            if (read_fd < 0) {
                read_fd = i;
            } else if (write_fd < 0) {
                write_fd = i;
            }
        }
    }
    
    if (read_fd < 0 || write_fd < 0) {
        pmm_free_page(pipe);
        return -1;
    }
    
    /* Assign FDs to pipe */
    current->fds[read_fd] = pipe;
    current->fds[write_fd] = pipe;
    fds[0] = read_fd;
    fds[1] = write_fd;
    
    serial_print("[PIPE] Pipe created: read_fd=");
    serial_print_dec(read_fd);
    serial_print(", write_fd=");
    serial_print_dec(write_fd);
    serial_print("\n");
    
    return 0;
}

int pipe_read(pipe_t* pipe, void* buf, u64 count) {
    if (!pipe || pipe->read_closed) return -1;
    
    if (count > pipe->count) {
        count = pipe->count;
    }
    
    u8* dst = (u8*)buf;
    for (u64 i = 0; i < count; i++) {
        dst[i] = pipe->buffer[pipe->read_pos];
        pipe->read_pos = (pipe->read_pos + 1) % PIPE_BUFFER_SIZE;
        pipe->count--;
    }
    
    return count;
}

int pipe_write(pipe_t* pipe, const void* buf, u64 count) {
    if (!pipe || pipe->write_closed) return -1;
    
    /* Check for space */
    u64 available = PIPE_BUFFER_SIZE - pipe->count;
    if (count > available) {
        count = available;
    }
    
    const u8* src = (const u8*)buf;
    for (u64 i = 0; i < count; i++) {
        pipe->buffer[pipe->write_pos] = src[i];
        pipe->write_pos = (pipe->write_pos + 1) % PIPE_BUFFER_SIZE;
        pipe->count++;
    }
    
    return count;
}

void pipe_close_read(pipe_t* pipe) {
    if (pipe) {
        pipe->read_closed = 1;
    }
}

void pipe_close_write(pipe_t* pipe) {
    if (pipe) {
        pipe->write_closed = 1;
    }
}
