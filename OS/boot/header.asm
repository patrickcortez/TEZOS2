section .multiboot_header
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
    dw 0       ; Flags
    dd 20      ; Size
    dd 0       ; Width (0 = preference)
    dd 0       ; Height (0 = preference)
    dd 32      ; Depth (32-bit)
    dd 0       ; Padding to 8-byte alignment (20 + 4 = 24)

    ; End tag
    dw 0
    dw 0
    dd 8
header_end:
