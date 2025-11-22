section .text
global _start
extern main
extern exit

_start:
    ; Stack is already set up by kernel exec
    ; argc, argv, envp are on stack (not implemented yet, but good to know)
    
    call main
    
    ; Pass return value to exit
    mov rdi, rax
    call exit
    
    ; Should not reach here
    hlt
