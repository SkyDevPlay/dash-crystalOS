[bits 32]
section .text

global _start
extern main

_start:
    ; Écrire 'A' directement dans le buffer VGA
    mov byte [0xB8000], 'A'
    mov byte [0xB8001], 0x0F

    mov esp, 0x90000
    mov ebp, esp
    call main
    jmp $
