#ifndef EXCEPTION_H
#define EXCEPTION_H

#include "types.h"

/* CPU Exception Numbers */
#define EXC_DIVIDE_ERROR        0
#define EXC_DEBUG               1
#define EXC_NMI                 2
#define EXC_BREAKPOINT          3
#define EXC_OVERFLOW            4
#define EXC_BOUND_RANGE         5
#define EXC_INVALID_OPCODE      6
#define EXC_DEVICE_NOT_AVAIL    7
#define EXC_DOUBLE_FAULT        8
#define EXC_COPROCESSOR         9
#define EXC_INVALID_TSS         10
#define EXC_SEGMENT_NOT_PRESENT 11
#define EXC_STACK_FAULT         12
#define EXC_GENERAL_PROTECTION  13
#define EXC_PAGE_FAULT          14
#define EXC_FPU_ERROR           16
#define EXC_ALIGNMENT_CHECK     17
#define EXC_MACHINE_CHECK       18
#define EXC_SIMD_FP_EXCEPTION   19

/* Exception Stack Frame (pushed by CPU) */
typedef struct {
    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
} __attribute__((packed)) exception_frame_t;

/* Full CPU State (for debugging) */
typedef struct {
    /* Segment registers */
    u64 ds, es, fs, gs;
    
    /* General purpose registers */
    u64 rax, rbx, rcx, rdx;
    u64 rsi, rdi, rbp, rsp;
    u64 r8, r9, r10, r11;
    u64 r12, r13, r14, r15;
    
    /* Exception info */
    u64 int_no;
    u64 err_code;
    
    /* CPU-pushed frame */
    u64 rip;
    u64 cs;
    u64 rflags;
    u64 user_rsp;
    u64 ss;
} __attribute__((packed)) cpu_state_t;

/* Exception handlers */
void exception_init(void);
void exception_handler(cpu_state_t* state);

/* Specific handlers */
void page_fault_handler(cpu_state_t* state);
void gpf_handler(cpu_state_t* state);
void double_fault_handler(cpu_state_t* state);

/* Stack trace */
void print_stack_trace(u64 rbp);
void dump_registers(cpu_state_t* state);

#endif
