#include "sys/lba.h"
#include "io.h"

#define ATA_DATA     0x1F0
#define ATA_ERR      0x1F1
#define ATA_COUNT    0x1F2
#define ATA_LBA_LO   0x1F3
#define ATA_LBA_MID  0x1F4
#define ATA_LBA_HI   0x1F5
#define ATA_DRIVE    0x1F6
#define ATA_STATUS   0x1F7
#define ATA_CONTROL  0x3F6

#define ATA_BSY      0x80
#define ATA_DRQ      0x08
#define ATA_ERR_BIT  0x01
#define ATA_TIMEOUT  2000000

#define PCI_ADDR     0xCF8
#define PCI_DATA     0xCFC

static void pci_write(u8 bus, u8 dev, u8 func, u8 reg, u32 val) {
    u32 addr = 0x80000000 | (bus<<16) | (dev<<11) | (func<<8) | (reg&0xFC);
    outl(PCI_ADDR, addr);
    outl(PCI_DATA, val);
}

static void ide_pci_init(void) {
    /* Activer le bus mastering et I/O sur le contrôleur IDE PCI (bus 0, dev 1, func 1) */
    pci_write(0, 1, 1, 0x04, 0x05);
    /* Mode compatibilité native désactivé — garder les ports 0x1F0 */
    pci_write(0, 1, 1, 0x08, 0x00);
}

static void ata_400ns(void) {
    inb(ATA_CONTROL);
    inb(ATA_CONTROL);
    inb(ATA_CONTROL);
    inb(ATA_CONTROL);
}

static int ata_wait_bsy(void) {
    for (int i = 0; i < ATA_TIMEOUT; i++) {
        u8 st = inb(ATA_STATUS);
        if (st == 0xFF) continue;
        if (!(st & ATA_BSY)) return 0;
    }
    return -1;
}

static int ata_wait_drq(void) {
    for (int i = 0; i < ATA_TIMEOUT; i++) {
        u8 st = inb(ATA_STATUS);
        if (st == 0xFF) continue;
        if (st & ATA_ERR_BIT) return -1;
        if ((st & ATA_DRQ) && !(st & ATA_BSY)) return 0;
    }
    return -1;
}

static int ata_init_done = 0;

static void ata_init(void) {
    if (ata_init_done) return;
    ata_init_done = 1;

    // Reset software
    outb(ATA_CONTROL, 0x04);
    for (int i = 0; i < 100000; i++) inb(ATA_CONTROL);
    outb(ATA_CONTROL, 0x00);
    for (int i = 0; i < 100000; i++) inb(ATA_CONTROL);

    // Sélectionner master drive
    outb(ATA_DRIVE, 0xA0);
    for (int i = 0; i < 100000; i++) inb(ATA_CONTROL);

    // Attendre que BSY se lève
    for (int i = 0; i < ATA_TIMEOUT; i++) {
        u8 st = inb(ATA_STATUS);
        if (st == 0xFF) continue;   // floating bus
        if (!(st & ATA_BSY)) break;
    }

    // IDENTIFY
    outb(ATA_DRIVE,  0xA0);
    outb(ATA_COUNT,  0);
    outb(ATA_LBA_LO, 0);
    outb(ATA_LBA_MID,0);
    outb(ATA_LBA_HI, 0);
    outb(ATA_STATUS, 0xEC);  // IDENTIFY command

    u8 status = inb(ATA_STATUS);
    if (status == 0 || status == 0xFF) {
        printf("ATA: no disk!\n");
        return;
    }

    if (ata_wait_drq() < 0) {
        printf("ATA: IDENTIFY timeout\n");
        return;
    }

    // Vider le buffer IDENTIFY (256 words)
    for (int i = 0; i < 256; i++) inw(ATA_DATA);
    printf("ATA: disk OK\n");
}

void ata_lba_read(u32 sector, u8 count, void *buf) {
    ata_init();
    if (ata_wait_bsy() < 0) return;

    outb(ATA_DRIVE,   0xE0 | ((sector >> 24) & 0x0F));
    ata_400ns();
    if (ata_wait_bsy() < 0) return;

    outb(ATA_ERR,     0x00);
    outb(ATA_COUNT,   count);
    outb(ATA_LBA_LO,  (sector)       & 0xFF);
    outb(ATA_LBA_MID, (sector >> 8)  & 0xFF);
    outb(ATA_LBA_HI,  (sector >> 16) & 0xFF);
    outb(ATA_DRIVE,   0xE0 | ((sector >> 24) & 0x0F));
    outb(ATA_STATUS,  0x20);

    for (int a = 0; a < count; a++) {
        if (ata_wait_drq() < 0) return;
        u16 *p = (u16*)buf;
        for (int i = 0; i < 256; i++)
            p[i] = inw(ATA_DATA);
        buf = (u8*)buf + 512;
    }
}

void ata_lba_write(u32 sector, u8 count, void *buf) {
    ata_init();
    if (ata_wait_bsy() < 0) return;

    outb(ATA_DRIVE,   0xE0 | ((sector >> 24) & 0x0F));
    ata_400ns();
    if (ata_wait_bsy() < 0) return;

    outb(ATA_ERR,     0x00);
    outb(ATA_COUNT,   count);
    outb(ATA_LBA_LO,  (sector)       & 0xFF);
    outb(ATA_LBA_MID, (sector >> 8)  & 0xFF);
    outb(ATA_LBA_HI,  (sector >> 16) & 0xFF);
    outb(ATA_DRIVE,   0xE0 | ((sector >> 24) & 0x0F));
    outb(ATA_STATUS,  0x30);

    for (int a = 0; a < count; a++) {
        if (ata_wait_drq() < 0) return;
        u16 *p = (u16*)buf;
        for (int i = 0; i < 256; i++)
            outw(ATA_DATA, p[i]);
        buf = (u8*)buf + 512;
    }
}
