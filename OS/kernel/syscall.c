#include "syscall.h"
#include "video.h"
#include "serial.h"
#include "idt.h"
#include "task.h"
#include "fs.h"
#include "string.h"
#include "pmm.h"
#include "vmm.h"

extern void print_str(const char* s);
extern void print_char(char c);
extern void syscall_entry(void);

/* Syscall dispatch table */
typedef u64 (*syscall_fn_t)(u64, u64, u64, u64, u64, u64);
static syscall_fn_t syscall_table[256] = {0};

/* === SYSCALL IMPLEMENTATIONS === */

u64 sys_exit(int code) {
    serial_print("[SYSCALL] exit(");
    serial_print_dec(code);
    serial_print(")\n");
    
    /* Terminate current task */
    task_exit(code);
    
    /* Never returns */
    return 0;
}

u64 sys_read(int fd, void* buf, u64 count) {
    if (fd == STDIN) {
        /* TODO: Implement keyboard input */
        return 0;
    }
    
    task_t* current = task_get_current();
    if (!current) return (u64)-1;
    
    if (fd < 0 || fd >= MAX_FDS) return (u64)-1;
    if (!current->fds[fd]) return (u64)-1;
    
    int bytes_read = fs_read(current->fds[fd], buf, count);
    return bytes_read >= 0 ? bytes_read : (u64)-1;
}

u64 sys_write(int fd, const void* buf, u64 count) {
    if (fd == STDOUT || fd == STDERR) {
        /* Write to console */
        const char* str = (const char*)buf;
        for (u64 i = 0; i < count; i++) {
            print_char(str[i]);
        }
        return count;
    }
    
    task_t* current = task_get_current();
    if (!current) return (u64)-1;
    
    if (fd < 0 || fd >= MAX_FDS) return (u64)-1;
    if (!current->fds[fd]) return (u64)-1;
    
    int bytes_written = fs_write(current->fds[fd], buf, count);
    return bytes_written >= 0 ? bytes_written : (u64)-1;
}

u64 sys_open(const char* path, int flags) {
    task_t* current = task_get_current();
    if (!current) return (u64)-1;
    
    /* Find free FD */
    int fd = -1;
    for (int i = 3; i < MAX_FDS; i++) {
        if (!current->fds[i]) {
            fd = i;
            break;
        }
    }
    
    if (fd < 0) return (u64)-1;
    
    fs_file_t* file = fs_open(path, flags);
    if (!file) return (u64)-1;
    
    current->fds[fd] = file;
    return fd;
}

u64 sys_close(int fd) {
    task_t* current = task_get_current();
    if (!current) return (u64)-1;
    
    if (fd < 3 || fd >= MAX_FDS) return (u64)-1;
    if (!current->fds[fd]) return (u64)-1;
    
    fs_close(current->fds[fd]);
    current->fds[fd] = 0;
    return 0;
}

u64 sys_getpid(void) {
    task_t* current = task_get_current();
    int pid = current ? current->pid : 0;
    return pid;
}

u64 sys_fork(void) {
    task_t* parent = task_get_current();
    if (!parent) return (u64)-1;
    
    serial_print("[SYSCALL] fork() - creating child process\n");
    
    /* Create new task */
    task_t* child = (task_t*)pmm_alloc_page();
    if (!child) return (u64)-1;
    
    /* Copy parent task structure */
    memcpy(child, parent, sizeof(task_t));
    
    /* Assign new PID */
    extern int next_pid;
    child->pid = next_pid++;
    child->ppid = parent->pid;
    child->state = TASK_READY;
    
    /* Allocate new kernel stack */
    child->kernel_stack = (u64)pmm_alloc_page();
    if (!child->kernel_stack) {
        pmm_free_page(child);
        return (u64)-1;
    }
    child->kernel_stack += KERNEL_STACK_SIZE;
    
    /* Create new address space */
    child->page_dir = vmm_create_address_space();
    if (!child->page_dir) {
        pmm_free_page((void*)(child->kernel_stack - KERNEL_STACK_SIZE));
        pmm_free_page(child);
        return (u64)-1;
    }
    
    /* Copy user space memory pages */
    for (u64 addr = 0x400000; addr < 0x80000000; addr += 0x1000) {
        u64 phys = vmm_get_physical_address(parent->page_dir, addr);
        if (phys) {
            void* new_page = pmm_alloc_page();
            if (new_page) {
                memcpy(new_page, (void*)phys, 0x1000);
                vmm_map_page(child->page_dir, addr, (u64)new_page, PAGE_WRITE | PAGE_USER);
            }
        }
    }
    
    /* Duplicate File Descriptors (Deep Copy for now) */
    for (int i = 0; i < MAX_FDS; i++) {
        if (parent->fds[i]) {
            fs_file_t* new_file = (fs_file_t*)kmalloc(sizeof(fs_file_t));
            if (new_file) {
                memcpy(new_file, parent->fds[i], sizeof(fs_file_t));
                child->fds[i] = new_file;
            } else {
                child->fds[i] = 0;
            }
        } else {
            child->fds[i] = 0;
        }
    }
    
    /* Add to task list */
    extern task_t* task_list;
    child->next = 0;
    if (!task_list) {
        task_list = child;
    } else {
        task_t* t = task_list;
        while (t->next) t = t->next;
        t->next = child;
    }
    
    serial_print("[SYSCALL] fork() created child  PID ");
    serial_print_dec(child->pid);
    serial_print("\n");
    
    return child->pid;
}

u64 sys_exec(const char* path) {
    serial_print("[SYSCALL] exec(\"");
    serial_print(path);
    serial_print("\")\n");
    
    extern int elf_exec(const char* path);
    int result = elf_exec(path);
    if (result < 0) {
        serial_print("[SYSCALL] exec failed\n");
        return (u64)-1;
    }
    return result;
}

u64 sys_waitpid(int pid, int* status, int options) {
    task_t* child = task_get_by_pid(pid);
    if (!child) return (u64)-1;
    
    task_t* current = task_get_current();
    if (child->ppid != current->pid) return (u64)-1;
    
    while (child->state != TASK_ZOMBIE) {
        task_yield();
    }
    
    if (status) {
        *status = child->exit_code;
    }
    
    scheduler_remove_task(child);
    pmm_free_page((void*)(child->kernel_stack - KERNEL_STACK_SIZE));
    vmm_destroy_address_space(child->page_dir);
    pmm_free_page(child);
    
    return pid;
}

u64 sys_brk(void* addr) {
    task_t* current = task_get_current();
    if (!current) return (u64)-1;
    
    if (current->heap_start == 0) {
        current->heap_start = 0x10000000;
        current->heap_end = current->heap_start;
    }
    
    u64 new_brk = (u64)addr;
    
    if (new_brk == 0) {
        return current->heap_end;
    }
    
    if (new_brk > current->heap_end) {
        u64 start_page = (current->heap_end + 0xFFF) & ~0xFFF;
        u64 end_page = (new_brk + 0xFFF) & ~0xFFF;
        
        for (u64 page = start_page; page < end_page; page += 0x1000) {
            void* phys = pmm_alloc_page();
            if (!phys) return current->heap_end;
            vmm_map_page(current->page_dir, page, (u64)phys, PAGE_WRITE | PAGE_USER);
        }
        current->heap_end = new_brk;
    } else if (new_brk < current->heap_end) {
        u64 start_page = (new_brk + 0xFFF) & ~0xFFF;
        u64 end_page = (current->heap_end + 0xFFF) & ~0xFFF;
        
        for (u64 page = start_page; page < end_page; page += 0x1000) {
            u64 phys = vmm_get_physical_address(current->page_dir, page);
            if (phys) pmm_free_page((void*)phys);
            vmm_unmap_page(current->page_dir, page);
        }
        current->heap_end = new_brk;
    }
    
    return current->heap_end;
}

u64 sys_mmap(void* addr, u64 length, int prot, int flags, int fd, u64 offset) {
    task_t* current = task_get_current();
    if (!current) return (u64)-1;
    
    if (current->mmap_base == 0) {
        current->mmap_base = 0x40000000;
    }
    
    u64 map_addr = addr ? (u64)addr : current->mmap_base;
    u64 pages = (length + 0xFFF) / 0x1000;
    
    for (u64 i = 0; i < pages; i++) {
        void* phys = pmm_alloc_page();
        if (!phys) return (u64)-1;
        
        u64 page_addr = map_addr + (i * 0x1000);
        vmm_map_page(current->page_dir, page_addr, (u64)phys, PAGE_WRITE | PAGE_USER);
        memset((void*)page_addr, 0, 0x1000);
    }
    
    current->mmap_base = map_addr + (pages * 0x1000);
    return map_addr;
}

u64 sys_munmap(void* addr, u64 length) {
    task_t* current = task_get_current();
    if (!current) return (u64)-1;
    
    u64 map_addr = (u64)addr & ~0xFFF;
    u64 pages = (length + 0xFFF) / 0x1000;
    
    for (u64 i = 0; i < pages; i++) {
        u64 page_addr = map_addr + (i * 0x1000);
        u64 phys = vmm_get_physical_address(current->page_dir, page_addr);
        if (phys) {
            pmm_free_page((void*)phys);
        }
        vmm_unmap_page(current->page_dir, page_addr);
    }
    
    return 0;
}

u64 sys_stat(const char* path, void* buf) {
    return fs_stat(path, (file_info_t*)buf);
}

u64 sys_mkdir(const char* path, int mode) {
    return fs_mkdir(path);
}

u64 sys_rmdir(const char* path) {
    return fs_rmdir(path);
}

u64 sys_chdir(const char* path) {
    return fs_chdir(path);
}

u64 sys_getcwd(char* buf, u64 size) {
    return fs_getcwd(buf, size);
}

u64 sys_rename(const char* oldpath, const char* newpath) {
    return fs_rename(oldpath, newpath);
}

u64 sys_readdir(int fd, void* entry) {
    task_t* current = task_get_current();
    if (!current) return (u64)-1;
    
    if (fd < 0 || fd >= MAX_FDS) return (u64)-1;
    fs_file_t* file = current->fds[fd];
    if (!file) return (u64)-1;
    
    if (!file->is_directory) return (u64)-1;
    
    dir_entry_t* out_entry = (dir_entry_t*)entry;
    if (fs_readdir_file(file, out_entry) == 0) {
        return 0;
    }
    return (u64)-1;
}

/* === SYSCALL DISPATCHER === */

u64 syscall_dispatcher(syscall_frame_t* frame) {
    u64 syscall_no = frame->rax;
    u64 arg1 = frame->rdi;
    u64 arg2 = frame->rsi;
    u64 arg3 = frame->rdx;
    u64 arg4 = frame->r10;
    u64 arg5 = frame->r8;
    u64 arg6 = frame->r9;
    
    if (syscall_no >= 256 || syscall_table[syscall_no] == 0) {
        serial_print("[SYSCALL] Invalid syscall: ");
        serial_print_dec(syscall_no);
        serial_print("\n");
        return (u64)-1;
    }
    
    u64 result = syscall_table[syscall_no](arg1, arg2, arg3, arg4, arg5, arg6);
    frame->rax = result;
    return result;
}

/* === INITIALIZATION === */

void syscall_init(void) {
    serial_print("[KERNEL] Installing syscall interface...\n");
    
    set_idt_gate(0x80, (u64)syscall_entry, 0x08, 0xEE);
    
    syscall_table[SYS_EXIT]    = (syscall_fn_t)sys_exit;
    syscall_table[SYS_FORK]    = (syscall_fn_t)sys_fork;
    syscall_table[SYS_READ]    = (syscall_fn_t)sys_read;
    syscall_table[SYS_WRITE]   = (syscall_fn_t)sys_write;
    syscall_table[SYS_OPEN]    = (syscall_fn_t)sys_open;
    syscall_table[SYS_CLOSE]   = (syscall_fn_t)sys_close;
    syscall_table[SYS_WAITPID] = (syscall_fn_t)sys_waitpid;
    syscall_table[SYS_EXEC]    = (syscall_fn_t)sys_exec;
    syscall_table[SYS_GETPID]  = (syscall_fn_t)sys_getpid;
    syscall_table[SYS_BRK]     = (syscall_fn_t)sys_brk;
    syscall_table[SYS_MMAP]    = (syscall_fn_t)sys_mmap;
    syscall_table[SYS_STAT]    = (syscall_fn_t)sys_stat;
    syscall_table[SYS_MKDIR]   = (syscall_fn_t)sys_mkdir;
    syscall_table[SYS_RMDIR]   = (syscall_fn_t)sys_rmdir;
    syscall_table[SYS_CHDIR]   = (syscall_fn_t)sys_chdir;
    syscall_table[SYS_GETCWD]  = (syscall_fn_t)sys_getcwd;
    syscall_table[SYS_RENAME]  = (syscall_fn_t)sys_rename;
    syscall_table[SYS_MUNMAP]  = (syscall_fn_t)sys_munmap;
    
    serial_print("[KERNEL] Syscall interface ready (int 0x80)\n");
}
