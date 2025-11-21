global load_idt
global irq0_handler
global irq1_handler
extern isr1_handler_c
extern timer_handler
extern keyboard_handler

section .text
bits 64

; Macro to push all registers
%macro pushaq 0
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
%endmacro

; Macro to pop all registers
%macro popaq 0
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
%endmacro

load_idt:
    lidt [rdi]
    ret

; Timer interrupt handler wrapper
irq0_handler:
    pushaq
    call timer_handler
    popaq
    iretq

; Keyboard interrupt handler wrapper
irq1_handler:
    pushaq
    call keyboard_handler
    popaq
    iretq
