#include "task.h"
#include "pmm.h"
#include "vmm.h"
#include "string.h"
#include "serial.h"

task_t* task_list = 0;
task_t* current_task = 0;
static task_t* idle_task = 0;
int next_pid = 1;

/* Idle task - runs when no other tasks ready */
static void idle_func(void) {
    while(1) {
        __asm__ volatile("hlt");
    }
}

void task_init(void) {
    serial_print("[TASK] Initializing task management...\n");
    
    /* Create idle task */
    idle_task = task_create(idle_func, 0);
    idle_task->state = TASK_READY;
    idle_task->priority = 99;  /* Lowest priority */
    
    current_task = idle_task;
    
    serial_print("[TASK] Idle task created (PID ");
    serial_print_dec(idle_task->pid);
    serial_print(")\n");
}

task_t* task_create(void (*entry)(void), int user_mode) {
    /* Allocate task structure */
    task_t* task = (task_t*)pmm_alloc_page();
    if (!task) return 0;
    
    memset(task, 0, sizeof(task_t));
    
    /* Assign PID */
    task->pid = next_pid++;
    task->ppid = current_task ? current_task->pid : 0;
    task->state = TASK_READY;
    task->priority = 10;
    task->timeslice = 10;  /* 10 ticks */
    
    /* Allocate kernel stack */
    task->kernel_stack = (u64)pmm_alloc_page();
    if (!task->kernel_stack) {
        pmm_free_page(task);
        return 0;
    }
    task->kernel_stack += KERNEL_STACK_SIZE;  /* Stack grows down */
    
    /* Create address space */
    task->page_dir = vmm_create_address_space();
    if (!task->page_dir) {
        pmm_free_page((void*)(task->kernel_stack - KERNEL_STACK_SIZE));
        pmm_free_page(task);
        return 0;
    }
    
    if (user_mode) {
        /* Allocate user stack */
        void* user_stack_phys = pmm_alloc_page();
        if (!user_stack_phys) {
            vmm_destroy_address_space(task->page_dir);
            pmm_free_page((void*)(task->kernel_stack - KERNEL_STACK_SIZE));
            pmm_free_page(task);
            return 0;
        }
        
        /* Map user stack */
        u64 user_stack_virt = USER_STACK_TOP - PAGE_SIZE;
        vmm_map_page(task->page_dir, user_stack_virt, (u64)user_stack_phys,
                     PAGE_WRITE | PAGE_USER);
        
        task->user_stack = USER_STACK_TOP;
        
        /* Set up user mode context */
        task->context.rip = (u64)entry;
        task->context.cs = 0x18 | 3;   /* User code segment (RPL=3) */
        task->context.ss = 0x20 | 3;   /* User data segment (RPL=3) */
        task->context.rflags = 0x202;  /* IF=1 */
        task->context.rsp = task->user_stack;
    } else {
        /* Kernel mode task */
        task->context.rip = (u64)entry;
        task->context.cs = 0x08;       /* Kernel code segment */
        task->context.ss = 0x10;       /* Kernel data segment */
        task->context.rflags = 0x202;
        task->context.rsp = task->kernel_stack;
    }
    
    /* Add to task list */
    if (!task_list) {
        task_list = task;
    } else {
        task_t* t = task_list;
        while (t->next) t = t->next;
        t->next = task;
    }
    
    serial_print("[TASK] Created task PID ");
    serial_print_dec(task->pid);
    serial_print(user_mode ? " (user)\n" : " (kernel)\n");
    
    return task;
}

void task_exit(int code) {
    if (!current_task) return;
    
    serial_print("[TASK] Task ");
    serial_print_dec(current_task->pid);
    serial_print(" exiting with code ");
    serial_print_dec(code);
    serial_print("\n");
    
    current_task->state = TASK_ZOMBIE;
    current_task->exit_code = code;
    
    /* Notify parent via SIGCHLD */
    extern int signal_send(int pid, int sig);
    if (current_task->ppid > 0) {
        signal_send(current_task->ppid, 17);  /* SIGCHLD */
    }
    
    /* Free user space memory pages */
    if (current_task->page_dir) {
        /* Free user pages (heap, stack, code) */
        for (u64 addr = 0x400000; addr < 0x80000000; addr += 0x1000) {
            u64 phys = vmm_get_physical_address(current_task->page_dir, addr);
            if (phys) {
                pmm_free_page((void*)phys);
            }
        }
    }
    
    /* Close all file descriptors */
    for (int i = 0; i < MAX_FDS; i++) {
        if (current_task->fds[i]) {
            fs_close(current_task->fds[i]);
            current_task->fds[i] = 0;
        }
    }
    
    /* Switch to another task */
    schedule();
}

void task_yield(void) {
    schedule();
}

task_t* task_get_current(void) {
    return current_task;
}

task_t* task_get_by_pid(int pid) {
    task_t* t = task_list;
    while (t) {
        if (t->pid == pid) return t;
        t = t->next;
    }
    return 0;
}

/* Scheduler */
static task_t* next_task_to_run(void) {
    task_t* best = idle_task;
    int best_priority = 999;
    
    task_t* t = task_list;
    while (t) {
        if (t->state == TASK_READY && t->priority < best_priority) {
            best = t;
            best_priority = t->priority;
        }
        t = t->next;
    }
    
    return best;
}

void schedule(void) {
    if (!current_task) return;
    
    /* Save current task state */
    if (current_task->state == TASK_RUNNING) {
        current_task->state = TASK_READY;
    }
    
    /* Find next task */
    task_t* next = next_task_to_run();
    if (!next) next = idle_task;
    
    if (next == current_task) {
        /* Same task, just continue */
        current_task->state = TASK_RUNNING;
        return;
    }
    
    /* Switch tasks */
    task_t* old_task = current_task;
    current_task = next;
    current_task->state = TASK_RUNNING;
    current_task->timeslice = 10;
    
    /* Switch page tables */
    vmm_switch_address_space(current_task->page_dir);
    
    /* Context switch */
    switch_context(&old_task->context, &current_task->context);
}

void scheduler_tick(void) {
    if (!current_task) return;
    
    if (current_task->timeslice > 0) {
        current_task->timeslice--;
    }
    
    current_task->total_time++;
    
    if (current_task->timeslice == 0) {
        /* Time quantum expired, schedule another task */
        schedule();
    }
}

void scheduler_init(void) {
    serial_print("[SCHEDULER] Initialized\n");
}

void scheduler_add_task(task_t* task) {
    /* Already added in task_create */
}

void scheduler_remove_task(task_t* task) {
    if (!task) return;
    
    /* Remove from list */
    if (task_list == task) {
        task_list = task->next;
    } else {
        task_t* t = task_list;
        while (t && t->next != task) t = t->next;
        if (t) t->next = task->next;
    }
}
