[bits 16]
[org 0x7c00]

BOOT_DISK        equ 0x800
KERNEL_LOCATION  equ 0x1000
UPPER_MEM        equ 0x802
EXTENDED_MEM     equ 0x804

xor ax, ax
mov ds, ax
mov es, ax
mov ss, ax
mov sp, 0x7C00
mov bp, sp

mov [BOOT_DISK], dl

mov ax, 0xE801
int 15h
cmp cx, 0
jne .mem_ok
mov cx, ax
mov dx, bx
.mem_ok:
mov word[UPPER_MEM], cx
mov word[EXTENDED_MEM], dx

mov ah, 0
mov al, 0x3
int 10h

mov si, booting_msg
call print

mov ax, 0x0000
mov es, ax
mov bx, KERNEL_LOCATION

mov ah, 0x02
mov al, 0x20
mov ch, 0x00
mov cl, 0x02
mov dh, 0x00
mov dl, [BOOT_DISK]
int 13h
jc disk_error

mov si, loaded_msg
call print

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

cli
lgdt [gdt_descriptor]
mov eax, cr0
or eax, 1
mov cr0, eax

jmp CODE_SEG:start_pm

disk_error:
    ; Print "ERR:"
    mov si, err_prefix
    call print

    ; Print AH (error code) in hex
    mov al, ah
    call print_hex

    ; Print space
    mov al, ' '
    mov ah, 0x0E
    mov bh, 0
    int 10h

    ; Print "DRV:"
    mov si, drv_prefix
    call print

    ; Print DL (drive number) in hex
    mov al, [BOOT_DISK]
    call print_hex

    mov si, disk_err_msg
    call print
    jmp $

print_hex:
    push ax
    shr al, 4
    and al, 0x0F
    call print_hex_digit
    pop ax
    and al, 0x0F
    call print_hex_digit
    ret

print_hex_digit:
    add al, '0'
    cmp al, '9'
    jle .low
    add al, 7
.low:
    mov ah, 0x0E
    mov bh, 0
    int 10h
    ret

err_prefix   db "ERR:", 0
drv_prefix   db "DRV:", 0

%include "boot/fprint.asm"

booting_msg  db "Booting...", 0
loaded_msg   db "Kernel OK!", 0
disk_err_msg db " DISK ERR", 0

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

    mov ebp, 0x500000
    mov esp, ebp

    jmp KERNEL_LOCATION

jmp $

times 446-($-$$) db 0

mbr_part_1:
    db 0x00
    db 0xff, 0xff, 0xff
    db 0x0B
    db 0xff, 0xff, 0xff
    dd 2048
    dd 65536
mbr_part_2:
    dd 0, 0, 0, 0
mbr_part_3:
    dd 0, 0, 0, 0
mbr_part_4:
    dd 0, 0, 0, 0

dw 0xAA55