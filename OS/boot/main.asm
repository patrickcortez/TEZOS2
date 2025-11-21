global start
extern long_mode_start

section .text
bits 32
start:
    ; Setup stack
    mov esp, stack_top

    ; Save Multiboot Info pointer (EBX)
    mov edi, ebx 

    ; Check for CPUID
    call check_cpuid
    ; Check for Long Mode
    call check_long_mode

    ; Setup Paging
    call setup_page_tables
    ; Enable Paging
    call enable_paging

    ; Load 64-bit GDT
    lgdt [gdt64.pointer]
    jmp gdt64.code_segment:long_mode_start

check_cpuid:
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    cmp eax, ecx
    je .no_cpuid
    ret
.no_cpuid:
    hlt

check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode
    ret
.no_long_mode:
    hlt

setup_page_tables:
    ; Map first 4GB using 4 PDP (L3) entries and 4 PD (L2) tables
    
    ; L3 Entry 0 -> L2 Table 0 (0-1GB)
    mov eax, page_table_l2_0
    or eax, 0b11
    mov [page_table_l3], eax

    ; L3 Entry 1 -> L2 Table 1 (1-2GB)
    mov eax, page_table_l2_1
    or eax, 0b11
    mov [page_table_l3 + 8], eax

    ; L3 Entry 2 -> L2 Table 2 (2-3GB)
    mov eax, page_table_l2_2
    or eax, 0b11
    mov [page_table_l3 + 16], eax

    ; L3 Entry 3 -> L2 Table 3 (3-4GB)
    mov eax, page_table_l2_3
    or eax, 0b11
    mov [page_table_l3 + 24], eax

    ; Point L4 to L3
    mov eax, page_table_l3
    or eax, 0b11
    mov [page_table_l4], eax

    ; Fill all 4 L2 tables (2048 entries total)
    ; We'll do this with a nested loop or just one big loop if we are careful with addresses
    
    mov ecx, 0 ; counter (0 to 2048)
    
.loop:
    mov eax, 0x200000 ; 2MiB
    mul ecx
    or eax, 0b10000011 ; present, writable, huge page
    
    ; Calculate which table and offset
    ; But since the tables are contiguous in memory (defined below), we can just treat them as one big array?
    ; No, 'resb' doesn't guarantee they are packed if we use labels.
    ; Let's map them individually in a loop 0-511 for each table.
    
    ; Actually, let's just use one loop and handle the offset manually? 
    ; No, simpler to loop 4 times or use a register for the table base.
    
    ; Let's stick to the loop 0-511 for each table to be safe.
    
    ; --- Table 0 ---
    mov eax, 0x200000
    mul ecx ; eax = physical address
    or eax, 0b10000011
    mov [page_table_l2_0 + ecx * 8], eax
    mov dword [page_table_l2_0 + ecx * 8 + 4], 0
    
    ; --- Table 1 ---
    ; Physical address = (ecx + 512) * 2MB
    push eax ; save previous low bits
    mov eax, ecx
    add eax, 512
    mov edx, 0x200000
    mul edx
    or eax, 0b10000011
    mov [page_table_l2_1 + ecx * 8], eax
    mov dword [page_table_l2_1 + ecx * 8 + 4], 0
    pop eax ; restore (not really needed but clean)

    ; --- Table 2 ---
    mov eax, ecx
    add eax, 1024
    mov edx, 0x200000
    mul edx
    or eax, 0b10000011
    mov [page_table_l2_2 + ecx * 8], eax
    mov dword [page_table_l2_2 + ecx * 8 + 4], 0

    ; --- Table 3 ---
    mov eax, ecx
    add eax, 1536
    mov edx, 0x200000
    mul edx
    or eax, 0b10000011
    mov [page_table_l2_3 + ecx * 8], eax
    mov dword [page_table_l2_3 + ecx * 8 + 4], 0

    inc ecx
    cmp ecx, 512
    jne .loop
    
    ret

enable_paging:
    ; pass page table location to cpu
    mov eax, page_table_l4
    mov cr3, eax

    ; enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; enable long mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    
    ; Restore Multiboot pointer to EBX for main64.asm
    mov ebx, edi
    
    ret

section .bss
align 4096
page_table_l4:
    resb 4096
page_table_l3:
    resb 4096
page_table_l2_0:
    resb 4096
page_table_l2_1:
    resb 4096
page_table_l2_2:
    resb 4096
page_table_l2_3:
    resb 4096
stack_bottom:
    resb 4096 * 4
stack_top:

section .rodata
gdt64:
    dq 0 ; zero entry
.code_segment: equ $ - gdt64
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) ; code segment
.pointer:
    dw $ - gdt64 - 1
    dq gdt64
