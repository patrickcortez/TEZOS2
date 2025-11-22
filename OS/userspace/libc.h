#ifndef LIBC_H
#define LIBC_H

#define NULL 0

typedef unsigned long long u64;
typedef long long i64;
typedef unsigned int u32;
typedef int i32;
typedef unsigned short u16;
typedef short i16;
typedef unsigned char u8;
typedef char i8;

/* Syscall Numbers (Must match kernel/syscall.h) */
#define SYS_EXIT    0
#define SYS_FORK    1
#define SYS_READ    2
#define SYS_WRITE   3
#define SYS_OPEN    4
#define SYS_CLOSE   5
#define SYS_WAITPID 6
#define SYS_EXEC    7
#define SYS_GETPID  8
#define SYS_BRK     9
#define SYS_MMAP    10
#define SYS_STAT    11
#define SYS_MKDIR   12
#define SYS_RMDIR   13
#define SYS_CHDIR   14
#define SYS_GETCWD  15
#define SYS_RENAME  16
#define SYS_MUNMAP  17
#define SYS_READDIR 18  /* New */

/* File Flags */
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2
#define O_CREAT  4
#define O_TRUNC  8
#define O_APPEND 16
#define O_EXCL   32

/* File Info */
typedef struct {
    char name[256];
    u32 size;
    u32 type; /* 1=File, 2=Dir */
} file_info_t;

/* Directory Entry */
typedef struct {
    char name[256];
    u32 size;
    u32 is_directory;
} dir_entry_t;

/* System Calls */
void exit(int code);
int fork();
int read(int fd, void* buf, int count);
int write(int fd, const void* buf, int count);
int open(const char* path, int flags);
int close(int fd);
int waitpid(int pid, int* status, int options);
int exec(const char* path);
int getpid();
void* sbrk(int increment);
int stat(const char* path, file_info_t* info);
int mkdir(const char* path);
int rmdir(const char* path);
int chdir(const char* path);
int getcwd(char* buf, int size);
int rename(const char* oldpath, const char* newpath);
int readdir(int fd, dir_entry_t* entry); /* Custom syscall for now */

/* String / Memory */
int strlen(const char* str);
int strcmp(const char* s1, const char* s2);
void strcpy(char* dst, const char* src);
void strcat(char* dst, const char* src);
void memset(void* ptr, int val, int size);
void memcpy(void* dst, const void* src, int size);
int atoi(const char* str);
void itoa(int num, char* str, int base);

/* I/O */
void printf(const char* fmt, ...);
void puts(const char* s);
char getchar();
void gets(char* buf, int max);

#endif
