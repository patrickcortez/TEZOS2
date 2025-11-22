#ifndef STRING_H
#define STRING_H

#include "types.h"

/* Memory operations */
void* memcpy(void* dest, const void* src, u64 n);
void* memset(void* s, int c, u64 n);
int memcmp(const void* s1, const void* s2, u64 n);

/* String operations */
u64 strlen(const char* s);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, u64 n);
char* strcat(char* dest, const char* src);
int strcmp(const char* s1, const char* s2);

#endif
