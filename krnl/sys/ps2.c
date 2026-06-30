#include "sys/pic.h"
#include "sys/ps2.h"
#include "sys/ports.h"
#include "sys/io.h"
#include "io.h"

struct IDT_entry IDT[256];

extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();

typedef struct __attribute__((packed)) {
    u32 edi;
    u32 esi;
    u32 ebp;
    u32 esp;
    u32 ebx;
    u32 edx;
    u32 ecx;
    u32 eax;
    u32 numero_exception;
    u32 error_code;
    u32 eip;
    u32 cs;
    u32 eflags;
} registers_t;

void (*int_table[20])(void) = {
    isr0,
    isr1,
    isr2,
    isr3,
    isr4,
    isr5,
    isr6,
    isr7,
    isr8,
    isr9,
    isr10,
    isr11,
    isr12,
    isr13,
    isr14,
    isr15,
    isr16,
    isr17,
    isr18,
    isr19
};

void isr_handle(registers_t *regs) {
    clearScreen();
    printf("Exception %d (error code: %d)\n", (*regs).numero_exception
                   , (*regs).error_code);
    while (1);
}


void int_handle() {
    puts("\n\nOH NO AN EXCEPTION\n\n");
    while (1);
}

void init_idt() {
    IDT[0x21].segment = 8;
    IDT[0x21].zero = 0;
    IDT[0x21].type = 0b10001110;
    IDT[0x21].offset_lower = (u32)keyboard_handler & 0xFFFF;
    IDT[0x21].offset_upper = ((u32)keyboard_handler & 0xFFFF0000) >> 16;

    for(int i = 0; i < 20; i++) {
        IDT[i].segment = 8;
        IDT[i].zero = 0;
        IDT[i].type = 0b10001110;
        IDT[i].offset_lower = (u32)int_table[i] & 0xFFFF;
        IDT[i].offset_upper = ((u32)int_table[i] & 0xFFFF0000) >> 16;
    }
    
    for(int i = 20; i < 0x20; i++) {
        IDT[i].segment = 8;
        IDT[i].zero = 0;
        IDT[i].type = 0b10001110;
        IDT[i].offset_lower = (u32)int_handler & 0xFFFF;
        IDT[i].offset_upper = ((u32)int_handler & 0xFFFF0000) >> 16;
}

    // Restart PICs
    outb(PIC1_COMMAND, 0x11); 
    outb(PIC2_COMMAND, 0x11); 

    // Start PIC1 at 0x20 and PIC2 at 0x28
    outb(PIC1_DATA, 0x20);
    outb(PIC2_DATA, 0x28);

    // Configure PIC Cascade mode
    outb(PIC1_DATA, 4);
    outb(PIC2_DATA, 2);

    // Set 8086/88 mode
    outb(PIC1_DATA, 0x1);
    outb(PIC2_DATA, 0x1);

    // Mask all interrupts
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);

    struct IDT_pointer idt_ptr;
    idt_ptr.offset = (u32)&IDT;
    idt_ptr.limit = sizeof(struct IDT_entry)*256;
    load_idt(&idt_ptr);
}

void kb_init() {
    outb(PIC1_DATA, 0b11111101); // Unmask IRQ1
}
