; ===========================================================================
; Professional Interrupt/Exception/Syscall Handlers
; ===========================================================================

[bits 64]
[section .text]

global load_idt
global irq0_handler
global irq1_handler

; Exception handlers (ISR 0-19)
global isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7, isr8
global isr10, isr11, isr12, isr13, isr14, isr16, isr17, isr18, isr19

; Syscall handler
global syscall_entry

extern exception_handler
extern syscall_dispatcher
extern keyboard_handler
extern timer_handler

; ---------------------------------------------------------------------------
; IDT Loader
; ---------------------------------------------------------------------------
load_idt:
    lidt [rdi]
    ret

; ---------------------------------------------------------------------------
; Macro: ISR without error code
; ---------------------------------------------------------------------------
%macro ISR_NOERRCODE 1
isr%1:
    cli
    push 0              ; Dummy error code
    push %1             ; Interrupt number
    jmp isr_common
%endmacro

; ---------------------------------------------------------------------------
; Macro: ISR with error code
; ---------------------------------------------------------------------------
%macro ISR_ERRCODE 1
isr%1:
    cli
    push %1             ; Interrupt number
    jmp isr_common
%endmacro

; ---------------------------------------------------------------------------
; Exception ISRs (0-19)
; ---------------------------------------------------------------------------
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19

; ---------------------------------------------------------------------------
; Common ISR Handler
; ---------------------------------------------------------------------------
isr_common:
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rdi
    push rsi
    push rbp
    push rdx
    push rcx
    push rbx
    push rax
    
    xor rax, rax
    mov ax, ds
    push rax
    mov ax, es
    push rax
    mov ax, fs
    push rax
    mov ax, gs
    push rax
    
    mov rdi, rsp
    call exception_handler
    
    pop rax
    mov gs, ax
    pop rax
    mov fs, ax
    pop rax
    mov es, ax
    pop rax
    mov ds, ax
    
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rbp
    pop rsi
    pop rdi
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    
    add rsp, 16
    iretq

; ---------------------------------------------------------------------------
; IRQ Handlers
; ---------------------------------------------------------------------------
irq0_handler:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    
    call timer_handler
    
    mov al, 0x20
    out 0x20, al
    
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    iretq

irq1_handler:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    
    call keyboard_handler
    
    mov al, 0x20
    out 0x20, al
    
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    iretq

; ---------------------------------------------------------------------------
; System Call Handler (int 0x80)
; ---------------------------------------------------------------------------
syscall_entry:
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    
    push r9
    push r8
    push r10
    push rdx
    push rsi
    push rdi
    push rax
    
    mov rdi, rsp
    call syscall_dispatcher
    
    pop rax
    add rsp, 48
    
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    iretq
