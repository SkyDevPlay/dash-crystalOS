print:
    mov al, [si]
    inc si
    cmp al, 0
    je .p_end
    mov ah, 0xe
    mov bh, 0
    mov bl, 0x3
    int 10h
    jmp print
.p_end:
    ret
    