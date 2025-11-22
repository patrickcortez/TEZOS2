#include "string.h"

void* memcpy(void* dest, const void* src, u64 n) {
    u8* d = (u8*)dest;
    const u8* s = (const u8*)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

void* memset(void* s, int c, u64 n) {
    u8* p = (u8*)s;
    while (n--) {
        *p++ = (u8)c;
    }
    return s;
}

int memcmp(const void* s1, const void* s2, u64 n) {
    const u8* p1 = (const u8*)s1;
    const u8* p2 = (const u8*)s2;
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

u64 strlen(const char* s) {
    u64 len = 0;
    while (*s++) len++;
    return len;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

char* strcat(char* dest, const char* src) {
    char* d = dest;
    while (*d) d++;
    while ((*d++ = *src++));
    return dest;
}

char* strncpy(char* dest, const char* src, u64 n) {
    char* d = dest;
    while (n > 0 && *src) {
        *d++ = *src++;
        n--;
    }
    while (n > 0) {
        *d++ = 0;
        n--;
    }
    return dest;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const u8*)s1 - *(const u8*)s2;
}
