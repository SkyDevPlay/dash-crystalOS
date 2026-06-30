[bits 32]

extern handle_keyboard
extern int_handle
extern isr_handle

global load_idt
global enable_interrupts
global keyboard_handler
global int_handler

global isr0
global isr1
global isr2
global isr3
global isr4
global isr5
global isr6
global isr7
global isr8
global isr9
global isr10
global isr11
global isr12
global isr13
global isr14
global isr15
global isr16
global isr17
global isr18
global isr19

load_idt:
    mov edx, [esp + 4]
    lidt [edx]
    ret

enable_interrupts:
    sti
    ret

keyboard_handler:
    pushad
    cld
    call handle_keyboard
    popad
    iretd

int_handler:
    pushad
    cld
    call int_handle
    popad
    iretd
    ret

isr_common_handler:
    pushad
    push esp
    call isr_handle
    add esp, 4
    add esp, 8
    popad
    iretd

isr0:
    push 0
    push 0
    jmp isr_common_handler

isr1:
    push 1
    push 1
    jmp isr_common_handler

isr2:
    push 2
    push 2
    jmp isr_common_handler

isr3:
    push 3
    push 3
    jmp isr_common_handler

isr4:
    push 4
    push 4
    jmp isr_common_handler

isr5:
    push 5
    push 5
    jmp isr_common_handler

isr6:
    push 6
    push 6
    jmp isr_common_handler

isr7:
    push 7
    push 7
    jmp isr_common_handler

isr8:
    push 8
    jmp isr_common_handler

isr9:
    push 9
    push 9
    jmp isr_common_handler

isr10:
    push 10
    jmp isr_common_handler

isr11:
    push 11
    jmp isr_common_handler

isr12:
    push 12
    jmp isr_common_handler

isr13:
    push 13
    jmp isr_common_handler

isr14:
    push 14
    jmp isr_common_handler

isr15:
    push 15
    push 15
    jmp isr_common_handler

isr16:
    push 16
    push 16
    jmp isr_common_handler

isr17:
    push 17
    jmp isr_common_handler
 
 isr18:
    push 18
    push 18
    jmp isr_common_handler

isr19:
    push 19
    push 19
    jmp isr_common_handler
