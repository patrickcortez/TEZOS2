#ifndef TASK_H
#define TASK_H

#include "types.h"
#include "vmm.h"
#include "fs.h"

#define KERNEL_STACK_SIZE 0x4000  /* 16KB */

#define MAX_PENDING_SIGNALS 32
#define MAX_FDS 16 // Keep MAX_FDS as it's used in task_t

/* Task states */
typedef enum {
    TASK_RUNNING = 0,
    TASK_READY,
    TASK_BLOCKED,
    TASK_ZOMBIE
} task_state_t;

/* Signal handler type */
typedef void (*signal_handler_fn_t)(int);

/* CPU registers (for context switching) */
typedef struct {
    u64 r15, r14, r13, r12, r11, r10, r9, r8;
    u64 rbp, rdi, rsi, rdx, rcx, rbx, rax;
    u64 rip, cs, rflags, rsp, ss;
} __attribute__((packed)) cpu_context_t;

/* Task Control Block (PCB) */
typedef struct task {
    int pid;                          /* Process ID */
    int ppid;                         /* Parent PID */
    task_state_t state;               /* Task state */
    
    /* CPU Context */
    cpu_context_t context;            /* Saved registers */
    
    /* Memory */
    page_table_t* page_dir;           /* Page directory (CR3) */
    u64 kernel_stack;                 /* Kernel stack base */
    u64 user_stack;                   /* User stack base */
    
    /* Scheduling */
    int priority;                     /* Priority (0 = highest) */
    u64 timeslice;                    /* Time quantum remaining */
    u64 total_time;                   /* Total CPU time used */
    
    /* Exit status */
    int exit_code;                    /* Process exit code */
    
    /* Signals */
    signal_handler_fn_t signal_handlers[32];  /* Signal handlers */
    int signal_queue[MAX_PENDING_SIGNALS];    /* Pending signals */
    int pending_signals;              /* Number of pending signals */
    
    /* Memory Management */
    u64 heap_start;                   /* Heap start address */
    u64 heap_end;                     /* Current heap break */
    u64 mmap_base;                    /* mmap region base */
    
    /* File Descriptors */
    fs_file_t* fds[MAX_FDS];          /* File descriptor table */
    
    /* Linked list */
    struct task* next;
} task_t;

/* Task management */
void task_init(void);
task_t* task_create(void (*entry)(void), int user_mode);
void task_exit(int code);
void task_yield(void);
task_t* task_get_current(void);
task_t* task_get_by_pid(int pid);

/* Scheduler */
void scheduler_init(void);
void scheduler_add_task(task_t* task);
void scheduler_remove_task(task_t* task);
void scheduler_tick(void);
void schedule(void);

/* Context switch (in assembly) */
extern void switch_context(cpu_context_t* old_ctx, cpu_context_t* new_ctx);

#endif
