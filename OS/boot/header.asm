section .multiboot_header
    ; Legacy Entry Point (Offset 0)
    bits 16
    jmp legacy_start
    align 8

header_start:
    ; Magic number (multiboot 2)
    dd 0xe85250d6
    ; Architecture 0 (protected mode i386)
    dd 0
    ; Header length
    dd header_end - header_start
    ; Checksum
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))

    ; Module alignment tag
    dw 6       ; Type
    dw 0       ; Flags
    dd 8       ; Size
    dd 0       ; Padding (8 is already aligned)

    ; Framebuffer tag
    dw 5       ; Type
    dw 0       ; Flags (0 = Optional)
    dd 20      ; Size
    dd 1024    ; Width
    dd 768     ; Height
    dd 32      ; Depth (32-bit)
    dd 0       ; Padding to 8-byte alignment (20 + 4 = 24)

    ; End tag
    dw 0
    dw 0
    dd 8
header_end:

    ; Legacy Mode Implementation
    bits 16
legacy_start:
    cli
    
    ; Enable A20 Line (Fast method)
    in al, 0x92
    or al, 2
    out 0x92, al
    
    ; Load GDT
    ; We are at 0x10000 (linear), so ds=0x1000.
    ; We need offset relative to 0x10000.
    ; gdt32_ptr is absolute (0x100XX).
    ; So we use (gdt32_ptr - 0x10000)
    lgdt [gdt32_ptr - 0x10000]
    
    ; Switch to Protected Mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    ; Jump to 32-bit code
    ; Target is absolute 0x100XX.
    ; We need a 32-bit offset jump in 16-bit mode.
    jmp dword 0x08:legacy_pm

    bits 32
legacy_pm:
    ; Setup segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Jump to kernel entry point (extern start)
    extern start
    jmp start

    ; GDT for 32-bit mode
    align 4
gdt32:
    dq 0                ; Null
    dq 0x00CF9A000000FFFF ; Code (Base=0, Limit=4GB, Exec/Read, 32-bit)
    dq 0x00CF92000000FFFF ; Data (Base=0, Limit=4GB, Read/Write)
gdt32_end:
gdt32_ptr:
    dw gdt32_end - gdt32 - 1
    dd gdt32
