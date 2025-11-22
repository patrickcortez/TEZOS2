#ifndef ELF_H
#define ELF_H

#include "types.h"

#define ELF_MAGIC 0x464C457F  /* "\x7FELF" */

/* ELF header */
typedef struct {
    u32 magic;
    u8 ei_class;
    u8 ei_data;
    u8 ei_version;
    u8 ei_osabi;
    u8 ei_abiversion;
    u8 ei_pad[7];
    u16 type;
    u16 machine;
    u32 version;
    u64 entry;
    u64 phoff;
    u64 shoff;
    u32 flags;
    u16 ehsize;
    u16 phentsize;
    u16 phnum;
    u16 shentsize;
    u16 shnum;
    u16 shstrndx;
} __attribute__((packed)) elf_header_t;

/* Program header */
typedef struct {
    u32 type;
    u32 flags;
    u64 offset;
    u64 vaddr;
    u64 paddr;
    u64 filesz;
    u64 memsz;
    u64 align;
} __attribute__((packed)) elf_program_header_t;

#define PT_LOAD 1

/* ELF loader */
int elf_load(const char* path, u64* entry_point);
int elf_exec(const char* path);

#endif
