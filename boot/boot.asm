[bits 16]
[org 0x7c00]

BOOT_DISK        equ 0x800
KERNEL_LOCATION  equ 0x1000
UPPER_MEM        equ 0x802
EXTENDED_MEM     equ 0x804
FAT_BS_SEG       equ 0x0900
FAT_TABLE_SEG    equ 0x0920
FAT_ROOT_SEG     equ 0x1120
PART_LBA         equ 2048

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

mov ah, 0x41
mov bx, 0x55AA
mov dl, 0x80
int 13h
jc .no_ext
cmp bx, 0xAA55
jne .no_ext
jmp .has_ext
.no_ext:
    mov si, no_ext_msg
    call print
    jmp $
.has_ext:

mov ax, FAT_BS_SEG
mov es, ax
mov word [dap.count],   1
mov word [dap.offset],  0
mov word [dap.segment], FAT_BS_SEG
mov dword [dap.lba_lo], PART_LBA
mov dword [dap.lba_hi], 0
mov ah, 0x42
mov dl, 0x80
mov si, dap
int 13h
jc disk_error

mov word [dap.count],   64
mov word [dap.offset],  0
mov word [dap.segment], FAT_TABLE_SEG
mov dword [dap.lba_lo], PART_LBA + 4
mov dword [dap.lba_hi], 0
mov ah, 0x42
mov dl, 0x80
mov si, dap
int 13h
jc disk_error

mov word [dap.count],   32
mov word [dap.offset],  0
mov word [dap.segment], FAT_ROOT_SEG
mov dword [dap.lba_lo], PART_LBA + 4 + 128
mov dword [dap.lba_hi], 0
mov ah, 0x42
mov dl, 0x80
mov si, dap
int 13h
jc disk_error

mov si, fs_loaded_msg
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
    mov al, ah
    shr al, 4
    add al, '0'
    cmp al, '9'
    jle .low
    add al, 7
.low:
    mov ah, 0x0E
    mov bh, 0
    int 10h
    mov si, disk_err_msg
    call print
    jmp $

dap:
    db 0x10
    db 0x00
.count:
    dw 0
.offset:
    dw 0
.segment:
    dw 0
.lba_lo:
    dd 0
.lba_hi:
    dd 0

%include "boot/fprint.asm"

booting_msg  db "Booting...", 0
loaded_msg   db "Kernel OK!", 0
fs_loaded_msg db "FS OK!", 0
no_ext_msg   db "No INT13 ext!", 0
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

    mov ebp, 0x90000
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