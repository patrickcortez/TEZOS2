#include "exception.h"
#include "video.h"
#include "serial.h"
#include "idt.h"

extern void print_str(const char* s);
extern void print_char(char c);

/* Exception handler prototypes (defined in interrupts.asm) */
extern void isr0(void);   /* Divide Error */
extern void isr1(void);   /* Debug */
extern void isr2(void);   /* NMI */
extern void isr3(void);   /* Breakpoint */
extern void isr4(void);   /* Overflow */
extern void isr5(void);   /* Bound Range */
extern void isr6(void);   /* Invalid Opcode */
extern void isr7(void);   /* Device Not Available */
extern void isr8(void);   /* Double Fault */
extern void isr10(void);  /* Invalid TSS */
extern void isr11(void);  /* Segment Not Present */
extern void isr12(void);  /* Stack Fault */
extern void isr13(void);  /* General Protection */
extern void isr14(void);  /* Page Fault */
extern void isr16(void);  /* FPU Error */
extern void isr17(void);  /* Alignment Check */
extern void isr18(void);  /* Machine Check */
extern void isr19(void);  /* SIMD FP Exception */

static const char* exception_messages[] = {
    "Divide Error",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 FPU Error",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception"
};

static void print_hex64(u64 val) {
    const char* hex = "0123456789ABCDEF";
    print_str("0x");
    for (int i = 60; i >= 0; i -= 4) {
        print_char(hex[(val >> i) & 0xF]);
    }
}

void dump_registers(cpu_state_t* state) {
    print_str("\n=== CPU STATE DUMP ===\n");
    
    print_str("RAX: "); print_hex64(state->rax);
    print_str("  RBX: "); print_hex64(state->rbx);
    print_str("\nRCX: "); print_hex64(state->rcx);
    print_str("  RDX: "); print_hex64(state->rdx);
    print_str("\nRSI: "); print_hex64(state->rsi);
    print_str("  RDI: "); print_hex64(state->rdi);
    print_str("\nRBP: "); print_hex64(state->rbp);
    print_str("  RSP: "); print_hex64(state->user_rsp);
    print_str("\nR8:  "); print_hex64(state->r8);
    print_str("  R9:  "); print_hex64(state->r9);
    print_str("\nR10: "); print_hex64(state->r10);
    print_str("  R11: "); print_hex64(state->r11);
    print_str("\nR12: "); print_hex64(state->r12);
    print_str("  R13: "); print_hex64(state->r13);
    print_str("\nR14: "); print_hex64(state->r14);
    print_str("  R15: "); print_hex64(state->r15);
    
    print_str("\n\nRIP: "); print_hex64(state->rip);
    print_str("  CS: "); print_hex64(state->cs);
    print_str("\nRFLAGS: "); print_hex64(state->rflags);
    print_str("  SS: "); print_hex64(state->ss);
    print_str("\n");
}

void print_stack_trace(u64 rbp) {
    print_str("\n=== STACK TRACE ===\n");
    
    u64* frame = (u64*)rbp;
    int depth = 0;
    
    while (frame && depth < 10) {
        u64 ret_addr = frame[1];
        
        print_str("  ["); 
        print_char('0' + depth);
        print_str("] ");
        print_hex64(ret_addr);
        print_str("\n");
        
        frame = (u64*)frame[0];
        depth++;
    }
}

void page_fault_handler(cpu_state_t* state) {
    /* Read CR2 to get faulting address */
    u64 faulting_addr;
    __asm__ volatile("mov %%cr2, %0" : "=r"(faulting_addr));
    
    print_str("\n*** PAGE FAULT ***\n");
    print_str("Faulting Address: ");
    print_hex64(faulting_addr);
    print_str("\nError Code: ");
    print_hex64(state->err_code);
    print_str("\n");
    
    /* Decode error code */
    print_str("  Present: "); print_str((state->err_code & 0x1) ? "Yes" : "No");
    print_str("\n  Access: "); print_str((state->err_code & 0x2) ? "Write" : "Read");
    print_str("\n  Mode: "); print_str((state->err_code & 0x4) ? "User" : "Supervisor");
    print_str("\n  Reserved: "); print_str((state->err_code & 0x8) ? "Yes" : "No");
    print_str("\n  Instruction Fetch: "); print_str((state->err_code & 0x10) ? "Yes" : "No");
    print_str("\n");
    
    dump_registers(state);
    print_stack_trace(state->rbp);
    
    serial_print("\n[KERNEL PANIC] Page Fault\n");
    
    print_str("\n*** KERNEL PANIC - HALTING ***\n");
    __asm__ volatile("cli; hlt");
    while(1);
}

void gpf_handler(cpu_state_t* state) {
    print_str("\n*** GENERAL PROTECTION FAULT ***\n");
    print_str("Error Code: ");
    print_hex64(state->err_code);
    print_str("\n");
    
    dump_registers(state);
    print_stack_trace(state->rbp);
    
    serial_print("\n[KERNEL PANIC] GPF\n");
    
    print_str("\n*** KERNEL PANIC - HALTING ***\n");
    __asm__ volatile("cli; hlt");
    while(1);
}

void double_fault_handler(cpu_state_t* state) {
    print_str("\n*** DOUBLE FAULT ***\n");
    print_str("This is a critical error - system is unstable!\n");
    
    dump_registers(state);
    
    serial_print("\n[KERNEL PANIC] Double Fault\n");
    
    print_str("\n*** KERNEL PANIC - HALTING ***\n");
    __asm__ volatile("cli; hlt");
    while(1);
}

void exception_handler(cpu_state_t* state) {
    u32 int_no = state->int_no;
    
    serial_print("\n[EXCEPTION] ");
    if (int_no < 20) {
        serial_print(exception_messages[int_no]);
    } else {
        serial_print("Unknown");
    }
    serial_print("\n");
    
    /* Special handlers for specific exceptions */
    if (int_no == EXC_PAGE_FAULT) {
        page_fault_handler(state);
        return;
    }
    
    if (int_no == EXC_GENERAL_PROTECTION) {
        gpf_handler(state);
        return;
    }
    
    if (int_no == EXC_DOUBLE_FAULT) {
        double_fault_handler(state);
        return;
    }
    
    /* Generic exception handler */
    print_str("\n*** EXCEPTION: ");
    if (int_no < 20) {
        print_str(exception_messages[int_no]);
    } else {
        print_str("Unknown");
    }
    print_str(" ***\n");
    
    print_str("Exception Number: ");
    print_hex64(int_no);
    print_str("\nError Code: ");
    print_hex64(state->err_code);
    print_str("\n");
    
    dump_registers(state);
    print_stack_trace(state->rbp);
    
    print_str("\n*** SYSTEM HALTED ***\n");
    __asm__ volatile("cli; hlt");
    while(1);
}

void exception_init(void) {
    serial_print("[KERNEL] Installing exception handlers...\n");
    
    /* Install all CPU exception handlers (0-19) */
    /* 0x8E = Present, Ring 0, Interrupt Gate */
    set_idt_gate(0, (u64)isr0, 0x08, 0x8E);
    set_idt_gate(1, (u64)isr1, 0x08, 0x8E);
    set_idt_gate(2, (u64)isr2, 0x08, 0x8E);
    set_idt_gate(3, (u64)isr3, 0x08, 0x8E);
    set_idt_gate(4, (u64)isr4, 0x08, 0x8E);
    set_idt_gate(5, (u64)isr5, 0x08, 0x8E);
    set_idt_gate(6, (u64)isr6, 0x08, 0x8E);
    set_idt_gate(7, (u64)isr7, 0x08, 0x8E);
    set_idt_gate(8, (u64)isr8, 0x08, 0x8E);
    set_idt_gate(10, (u64)isr10, 0x08, 0x8E);
    set_idt_gate(11, (u64)isr11, 0x08, 0x8E);
    set_idt_gate(12, (u64)isr12, 0x08, 0x8E);
    set_idt_gate(13, (u64)isr13, 0x08, 0x8E);
    set_idt_gate(14, (u64)isr14, 0x08, 0x8E);
    set_idt_gate(16, (u64)isr16, 0x08, 0x8E);
    set_idt_gate(17, (u64)isr17, 0x08, 0x8E);
    set_idt_gate(18, (u64)isr18, 0x08, 0x8E);
    set_idt_gate(19, (u64)isr19, 0x08, 0x8E);
    
    serial_print("[KERNEL] Exception handlers installed\n");
}
