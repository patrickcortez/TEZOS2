; Context switching assembly
[bits 64]
[section .text]

global switch_context

; switch_context(cpu_context_t* old_ctx, cpu_context_t* new_ctx)
; RDI = old context
; RSI = new context
switch_context:
    ; Save old context
    mov [rdi + 0], r15
    mov [rdi + 8], r14
    mov [rdi + 16], r13
    mov [rdi + 24], r12
    mov [rdi + 32], r11
    mov [rdi + 40], r10
    mov [rdi + 48], r9
    mov [rdi + 56], r8
    mov [rdi + 64], rbp
    ; Skip rdi (it's our parameter)
    mov [rdi + 80], rsi
    mov [rdi + 88], rdx
    mov [rdi + 96], rcx
    mov [rdi + 104], rbx
    mov [rdi + 112], rax
    
    ; Save RIP (return address)
    mov rax, [rsp]
    mov [rdi + 120], rax
    
    ; Save CS
    mov ax, cs
    mov [rdi + 128], rax
    
    ; Save RFLAGS
    pushfq
    pop rax
    mov [rdi + 136], rax
    
    ; Save RSP
    lea rax, [rsp + 8]  ; Adjust for return address
    mov [rdi + 144], rax
    
    ; Save SS
    mov ax, ss
    mov [rdi + 152], rax
    
    ; Load new context
    mov r15, [rsi + 0]
    mov r14, [rsi + 8]
    mov r13, [rsi + 16]
    mov r12, [rsi + 24]
    mov r11, [rsi + 32]
    mov r10, [rsi + 40]
    mov r9, [rsi + 48]
    mov r8, [rsi + 56]
    mov rbp, [rsi + 64]
    ; RDI will be loaded last
    mov rdx, [rsi + 88]
    mov rcx, [rsi + 96]
    mov rbx, [rsi + 104]
    mov rax, [rsi + 112]
    
    ; Load new stack
    mov rsp, [rsi + 144]
    
    ; Push return context for iretq
    push qword [rsi + 152]  ; SS
    push qword [rsi + 144]  ; RSP
    push qword [rsi + 136]  ; RFLAGS
    push qword [rsi + 128]  ; CS
    push qword [rsi + 120]  ; RIP
    
    ; Load remaining registers
    mov rdi, [rsi + 72]
    mov rsi, [rsi + 80]
    
    ; Jump to new task
    iretq
