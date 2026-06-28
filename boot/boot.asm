[bits 16]
[org 0x7c00]

BOOT_DISK equ 0x800
KERNEL_LOCATION equ 0x1000

UPPER_MEM equ 0x802
EXTENDED_MEM equ 0x804


mov [BOOT_DISK], dl

mov ax, 0xE801
int 15h
mov word[UPPER_MEM], cx
mov word[EXTENDED_MEM], dx

mov sp, 0x4000
mov bp, 0x4000

mov ah, 0
mov al, 0x3
int 10h

mov ah, 0x2
mov al, 0x7F
mov ch, 0x0
mov dh, 0x0
mov cl, 0x2
mov bx, KERNEL_LOCATION
mov dl, [BOOT_DISK]
int 13h

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

cli
lgdt [gdt_descriptor]
mov eax, cr0
or eax, 1
mov cr0, eax

jmp CODE_SEG:start_pm

%include "boot/fprint.asm"

shell: db "D$>> ", 0

gdt_start:
    gdt_null:
    dd 0
    dd 0
    gdt_code:
    dw 0xFFFF
    dw 0x0
    db 0x0
    db 0b10011011
    db 0b11001111
    db 0x0
    gdt_data:
    dw 0xFFFF
    dw 0
    db 0
    db 0b10010011
    db 0b11001111
    db 0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

[bits 32]
start_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x90000
    mov esp, ebp

    jmp KERNEL_LOCATION

jmp $

times 446-($-$$) db 0

mbr_part_1:
    db 0x00
    db 0xff, 0xff, 0xff
    db 0x83
    db 0xff, 0xff, 0xff
    dd 2048
    dd 32768
mbr_part_2:
    dd 0, 0, 0, 0
mbr_part_3:
    dd 0, 0, 0, 0
mbr_part_4:
    dd 0, 0, 0, 0
    
dw 0xAA55

