#include "sys/lba.h"
#include "io.h"

#define ATA_PRIMARY  0x1F0
#define ATA_CONTROL  0x3F6
#define ATA_STATUS   (ATA_PRIMARY + 7)
#define ATA_DATA     (ATA_PRIMARY + 0)

#define ATA_BSY  0x80
#define ATA_DRQ  0x08
#define ATA_ERR  0x01

#define ATA_TIMEOUT 1000000

static void ata_reset(void) {
    outb(ATA_CONTROL, 0x04);
    for (int i = 0; i < 10000; i++) inb(ATA_CONTROL);
    outb(ATA_CONTROL, 0x00);
    for (int i = 0; i < 10000; i++) inb(ATA_CONTROL);
}

static u8 ata_wait_bsy(void) {
    for (int i = 0; i < ATA_TIMEOUT; i++) {
        u8 st = inb(ATA_STATUS);
        if (st == 0xFF) continue;
        if (!(st & ATA_BSY)) return st;
    }
    return 0xFF;
}

static u8 ata_wait_drq(void) {
    for (int i = 0; i < ATA_TIMEOUT; i++) {
        u8 st = inb(ATA_STATUS);
        if (st == 0xFF) continue;
        if (st & ATA_ERR) return 0xFF;
        if (!(st & ATA_BSY) && (st & ATA_DRQ)) return st;
    }
    return 0xFF;
}

void ata_lba_read(u32 sector, u8 count, void *buf) {
    ata_reset();
    if (ata_wait_bsy() == 0xFF) return;

    outb(ATA_PRIMARY + 6, 0xE0 | ((sector >> 24) & 0x0F));
    for (int i = 0; i < 1000; i++) inb(ATA_STATUS);

    outb(ATA_PRIMARY + 1, 0x00);
    outb(ATA_PRIMARY + 2, count);
    outb(ATA_PRIMARY + 3,  sector        & 0xFF);
    outb(ATA_PRIMARY + 4, (sector >>  8) & 0xFF);
    outb(ATA_PRIMARY + 5, (sector >> 16) & 0xFF);
    outb(ATA_PRIMARY + 6, 0xE0 | ((sector >> 24) & 0x0F));
    outb(ATA_PRIMARY + 7, 0x20);

    for (int a = 0; a < count; a++) {
        if (ata_wait_drq() == 0xFF) return;
        for (char *end = (char*)buf + 512; buf != (void*)end; buf += 2)
            *(u16*)buf = inw(ATA_DATA);
    }
}

void ata_lba_write(u32 sector, u8 count, void *buf) {
    ata_reset();
    if (ata_wait_bsy() == 0xFF) return;

    outb(ATA_PRIMARY + 6, 0xE0 | ((sector >> 24) & 0x0F));
    for (int i = 0; i < 1000; i++) inb(ATA_STATUS);

    outb(ATA_PRIMARY + 1, 0x00);
    outb(ATA_PRIMARY + 2, count);
    outb(ATA_PRIMARY + 3,  sector        & 0xFF);
    outb(ATA_PRIMARY + 4, (sector >>  8) & 0xFF);
    outb(ATA_PRIMARY + 5, (sector >> 16) & 0xFF);
    outb(ATA_PRIMARY + 6, 0xE0 | ((sector >> 24) & 0x0F));
    outb(ATA_PRIMARY + 7, 0x30);

    for (int a = 0; a < count; a++) {
        if (ata_wait_drq() == 0xFF) return;
        for (char *end = (char*)buf + 512; buf != (void*)end; buf += 2)
            outw(ATA_DATA, *(u16*)buf);
    }
}
