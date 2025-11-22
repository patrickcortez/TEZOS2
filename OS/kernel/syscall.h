#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"

/* System Call Numbers */
#define SYS_EXIT        1
#define SYS_FORK        2
#define SYS_READ        3
#define SYS_WRITE       4
#define SYS_OPEN        5
#define SYS_CLOSE       6
#define SYS_WAITPID     7
#define SYS_EXEC        11
#define SYS_GETPID      20
#define SYS_BRK         45
#define SYS_RENAME      16
#define SYS_MUNMAP      17
#define SYS_READDIR     18
#define SYS_MMAP        19
#define SYS_STAT        4
#define SYS_MKDIR       83
#define SYS_RMDIR       84
#define SYS_CHDIR       80
#define SYS_GETCWD      79
/* File Descriptor Constants */
#define STDIN   0
#define STDOUT  1
#define STDERR  2

/* Open flags */
#define O_RDONLY    0x0000
#define O_WRONLY    0x0001
#define O_RDWR      0x0002
#define O_CREAT     0x0040
#define O_TRUNC     0x0200
#define O_APPEND    0x0400

/* Syscall registers (x86-64 ABI) */
typedef struct {
    u64 rax;  /* Syscall number / Return value */
    u64 rdi;  /* Arg 1 */
    u64 rsi;  /* Arg 2 */
    u64 rdx;  /* Arg 3 */
    u64 r10;  /* Arg 4 */
    u64 r8;   /* Arg 5 */
    u64 r9;   /* Arg 6 */
    
    /* Saved by CPU */
    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
} __attribute__((packed)) syscall_frame_t;

/* Syscall initialization */
void syscall_init(void);

/* Syscall dispatcher */
u64 syscall_dispatcher(syscall_frame_t* frame);

/* Individual syscall implementations */
u64 sys_exit(int code);
u64 sys_fork(void);
u64 sys_read(int fd, void* buf, u64 count);
u64 sys_write(int fd, const void* buf, u64 count);
u64 sys_open(const char* path, int flags);
u64 sys_close(int fd);
u64 sys_waitpid(int pid, int* status, int options);
u64 sys_exec(const char* path);
u64 sys_getpid(void);
u64 sys_brk(void* addr);
u64 sys_mmap(void* addr, u64 length, int prot, int flags, int fd, u64 offset);
u64 sys_stat(const char* path, void* buf);
u64 sys_mkdir(const char* path, int mode);
u64 sys_rmdir(const char* path);
u64 sys_chdir(const char* path);
u64 sys_getcwd(char* buf, u64 size);
u64 sys_rename(const char* oldpath, const char* newpath);

#endif
