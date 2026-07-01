[bits 32]
section .text

global _start
extern main
extern _bss_start
extern _bss_end

_start:
    ; Écrire 'A' directement dans le buffer VGA
    mov byte [0xB8000], 'A'
    mov byte [0xB8001], 0x0F

    mov edi, _bss_start
    mov ecx, _bss_end
    sub ecx, edi
    shr ecx, 2
    xor eax, eax
    cld
    rep stosd

    mov esp, 0x500000
    mov ebp, esp
    call main
    jmp $
