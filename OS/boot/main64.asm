global long_mode_start
extern kmain

section .text
bits 64
long_mode_start:
    ; load 0 into all data segment registers
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Pass Multiboot info (in ebx from GRUB, moved to rdi for C ABI)
    ; Note: We need to ensure rbx is preserved during the 32->64 switch
    ; In main.asm, we should save ebx. Let's assume ebx is preserved or passed.
    ; Actually, in main.asm we need to pass it.
    
    mov rdi, rbx ; Argument 1 for kmain
    call kmain
    hlt
