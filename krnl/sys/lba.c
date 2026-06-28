#include "sys/lba.h"
#include "io.h"

#define ATA_DATA     0x1F0
#define ATA_FEATURE  0x1F1
#define ATA_COUNT    0x1F2
#define ATA_LBA_LO   0x1F3
#define ATA_LBA_MID  0x1F4
#define ATA_LBA_HI   0x1F5
#define ATA_DRIVE    0x1F6
#define ATA_CMD      0x1F7
#define ATA_ALT_ST   0x3F6
#define ATA_CONTROL  0x3F6

#define ATA_SR_BSY   0x80
#define ATA_SR_DRDY  0x40
#define ATA_SR_DRQ   0x08
#define ATA_SR_ERR   0x01
#define ATA_TIMEOUT  0x400000

static void ata_delay(void) {
    inb(ATA_ALT_ST); inb(ATA_ALT_ST);
    inb(ATA_ALT_ST); inb(ATA_ALT_ST);
}

static int ata_wait_ready(void) {
    for (int i = 0; i < ATA_TIMEOUT; i++) {
        u8 st = inb(ATA_ALT_ST);
        if (st == 0xFF) return -1;
        if (!(st & ATA_SR_BSY) && (st & ATA_SR_DRDY)) return 0;
    }
    return -1;
}

static int ata_poll(void) {
    for (int i = 0; i < ATA_TIMEOUT; i++) {
        u8 st = inb(ATA_ALT_ST);
        if (st == 0xFF) return -1;
        if (st & ATA_SR_ERR) return -1;
        if (!(st & ATA_SR_BSY) && (st & ATA_SR_DRQ)) return 0;
    }
    return -1;
}

void ata_lba_read(u32 sector, u8 count, void *buf) {
    outb(ATA_DRIVE,   0xE0 | ((sector >> 24) & 0x0F));
    ata_delay();
    if (ata_wait_ready() < 0) return;

    outb(ATA_FEATURE, 0x00);
    outb(ATA_COUNT,   count);
    outb(ATA_LBA_LO,  (sector)       & 0xFF);
    outb(ATA_LBA_MID, (sector >> 8)  & 0xFF);
    outb(ATA_LBA_HI,  (sector >> 16) & 0xFF);
    outb(ATA_DRIVE,   0xE0 | ((sector >> 24) & 0x0F));
    outb(ATA_CMD,     0x20);

    for (int a = 0; a < count; a++) {
        if (ata_poll() < 0) return;
        u16 *p = (u16 *)buf;
        for (int i = 0; i < 256; i++)
            p[i] = inw(ATA_DATA);
        buf = (u8 *)buf + 512;
    }
}

void ata_lba_write(u32 sector, u8 count, void *buf) {
    outb(ATA_DRIVE,   0xE0 | ((sector >> 24) & 0x0F));
    ata_delay();
    if (ata_wait_ready() < 0) return;

    outb(ATA_FEATURE, 0x00);
    outb(ATA_COUNT,   count);
    outb(ATA_LBA_LO,  (sector)       & 0xFF);
    outb(ATA_LBA_MID, (sector >> 8)  & 0xFF);
    outb(ATA_LBA_HI,  (sector >> 16) & 0xFF);
    outb(ATA_DRIVE,   0xE0 | ((sector >> 24) & 0x0F));
    outb(ATA_CMD,     0x30);

    for (int a = 0; a < count; a++) {
        if (ata_poll() < 0) return;
        u16 *p = (u16 *)buf;
        for (int i = 0; i < 256; i++)
            outw(ATA_DATA, p[i]);
        buf = (u8 *)buf + 512;
    }
}
