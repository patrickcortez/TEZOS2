#include "libc.h"

/* Inline Assembly for Syscalls */
static inline u64 syscall0(u64 n) {
    u64 ret;
    __asm__ volatile ("syscall" : "=a"(ret) : "a"(n) : "rcx", "r11", "memory");
    return ret;
}

static inline u64 syscall1(u64 n, u64 a1) {
    u64 ret;
    __asm__ volatile ("syscall" : "=a"(ret) : "a"(n), "D"(a1) : "rcx", "r11", "memory");
    return ret;
}

static inline u64 syscall2(u64 n, u64 a1, u64 a2) {
    u64 ret;
    __asm__ volatile ("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2) : "rcx", "r11", "memory");
    return ret;
}

static inline u64 syscall3(u64 n, u64 a1, u64 a2, u64 a3) {
    u64 ret;
    __asm__ volatile ("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2), "d"(a3) : "rcx", "r11", "memory");
    return ret;
}

/* Wrappers */
void exit(int code) { syscall1(SYS_EXIT, code); }
int fork() { return syscall0(SYS_FORK); }
int read(int fd, void* buf, int count) { return syscall3(SYS_READ, fd, (u64)buf, count); }
int write(int fd, const void* buf, int count) { return syscall3(SYS_WRITE, fd, (u64)buf, count); }
int open(const char* path, int flags) { return syscall2(SYS_OPEN, (u64)path, flags); }
int close(int fd) { return syscall1(SYS_CLOSE, fd); }
int waitpid(int pid, int* status, int options) { return syscall3(SYS_WAITPID, pid, (u64)status, options); }
int exec(const char* path) { return syscall1(SYS_EXEC, (u64)path); }
int getpid() { return syscall0(SYS_GETPID); }
void* sbrk(int increment) { 
    /* Simple sbrk using BRK syscall */
    /* First get current brk */
    u64 current = syscall1(SYS_BRK, 0);
    if (increment == 0) return (void*)current;
    
    u64 new_brk = syscall1(SYS_BRK, current + increment);
    if (new_brk == current) return (void*)-1; /* Failed */
    return (void*)current;
}
int stat(const char* path, file_info_t* info) { return syscall2(SYS_STAT, (u64)path, (u64)info); }
int mkdir(const char* path) { return syscall2(SYS_MKDIR, (u64)path, 0); }
int rmdir(const char* path) { return syscall1(SYS_RMDIR, (u64)path); }
int chdir(const char* path) { return syscall1(SYS_CHDIR, (u64)path); }
int getcwd(char* buf, int size) { return syscall2(SYS_GETCWD, (u64)buf, size); }
int rename(const char* oldpath, const char* newpath) { return syscall2(SYS_RENAME, (u64)oldpath, (u64)newpath); }
int readdir(int fd, dir_entry_t* entry) { return syscall2(SYS_READDIR, fd, (u64)entry); }

/* String / Memory */
int strlen(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

void strcpy(char* dst, const char* src) {
    while ((*dst++ = *src++));
}

void strcat(char* dst, const char* src) {
    while (*dst) dst++;
    while ((*dst++ = *src++));
}

void memset(void* ptr, int val, int size) {
    u8* p = (u8*)ptr;
    while (size--) *p++ = val;
}

void memcpy(void* dst, const void* src, int size) {
    u8* d = (u8*)dst;
    const u8* s = (const u8*)src;
    while (size--) *d++ = *s++;
}

int atoi(const char* str) {
    int res = 0;
    while (*str >= '0' && *str <= '9') {
        res = res * 10 + (*str - '0');
        str++;
    }
    return res;
}

void itoa(int num, char* str, int base) {
    int i = 0;
    int is_neg = 0;
    
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }
    
    if (num < 0 && base == 10) {
        is_neg = 1;
        num = -num;
    }
    
    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }
    
    if (is_neg) str[i++] = '-';
    str[i] = '\0';
    
    /* Reverse */
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

/* I/O */
void puts(const char* s) {
    write(1, s, strlen(s));
    write(1, "\n", 1);
}

void printf(const char* fmt, ...) {
    /* Very basic printf */
    /* Note: varargs not fully implemented in this minimal env, 
       assuming standard calling convention (registers) */
    /* Actually, implementing varargs without stdarg.h is tricky in C.
       For now, let's just support manual string building or basic cases.
       Wait, we can use __builtin_va_list if using GCC */
    
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    
    char buf[32];
    const char* p = fmt;
    
    while (*p) {
        if (*p == '%') {
            p++;
            if (*p == 's') {
                const char* s = __builtin_va_arg(args, const char*);
                write(1, s, strlen(s));
            } else if (*p == 'd') {
                int d = __builtin_va_arg(args, int);
                itoa(d, buf, 10);
                write(1, buf, strlen(buf));
            } else if (*p == 'x') {
                int x = __builtin_va_arg(args, int);
                itoa(x, buf, 16);
                write(1, buf, strlen(buf));
            } else if (*p == 'c') {
                int c = __builtin_va_arg(args, int);
                char ch = (char)c;
                write(1, &ch, 1);
            }
        } else {
            write(1, p, 1);
        }
        p++;
    }
    
    __builtin_va_end(args);
}

char getchar() {
    char c;
    if (read(0, &c, 1) == 1) return c;
    return 0;
}

void gets(char* buf, int max) {
    int i = 0;
    char c;
    while (i < max - 1) {
        c = getchar();
        if (c == '\n' || c == '\r') break;
        if (c == '\b') {
            if (i > 0) {
                i--;
                /* Echo backspace */
                write(1, "\b \b", 3);
            }
        } else if (c) {
            buf[i++] = c;
            write(1, &c, 1);
        }
    }
    buf[i] = 0;
    write(1, "\n", 1);
}
