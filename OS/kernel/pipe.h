#ifndef PIPE_H
#define PIPE_H

#include "types.h"

#define PIPE_BUFFER_SIZE 4096

typedef struct {
    u8 buffer[PIPE_BUFFER_SIZE];
    u32 read_pos;
    u32 write_pos;
    u32 count;
    int read_closed;
    int write_closed;
} pipe_t;

/* Pipe operations */
int pipe_create(int fds[2]);
int pipe_read(pipe_t* pipe, void* buf, u64 count);
int pipe_write(pipe_t* pipe, const void* buf, u64 count);
void pipe_close_read(pipe_t* pipe);
void pipe_close_write(pipe_t* pipe);

#endif
