#include "sys/serial.h"
#include "sys/io.h"
#include "sys/ports.h"

u16 coms_table[4] = { 0x3F8, 0x2F8, 0x3E8, 0x2E8 };
u16 lpts_table[3] = { 0x378, 0x278, 0x3BC };
u16 *coms = coms_table;
u16 *lpts = lpts_table;

int initSerial(u16 port) {
    outb(port + 1, 0x00);
    outb(port + 3, 0x80);
    outb(port + 0, 0x03);
    outb(port + 1, 0x00);
    outb(port + 3, 0x03);
    outb(port + 2, 0xC7);
    outb(port + 4, 0x0B);
    outb(port + 4, 0x1E);
    outb(port + 0, 0xAE);

    if (inb(port + 0) != 0xAE) return -1;
    outb(port + 4, 0x0F);
    return 0;
}

void writeSerial(u16 port, u8 c) {
    while (!(inb(port + 5) & 0x20));
    outb(port, c);
}

u8 readSerial(u16 port) {
    while (!(inb(port + 5) & 0x01));
    return inb(port);
}

void serialString(u16 port, char *str) {
    while (*str) writeSerial(port, (u8)*str++);
}