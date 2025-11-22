; GDT/TSS loading
[bits 64]
[section .text]

global gdt_flush
global tss_flush

gdt_flush:
    lgdt [rdi]
    
    ; Reload segment registers
    mov ax, 0x10      ; Kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Far return to reload CS
    pop rdi           ; Get return address
    push 0x08         ; Kernel code segment
    push rdi          ; Return address
    retfq

tss_flush:
    mov ax, 0x28      ; TSS segment
    ltr ax
    ret
