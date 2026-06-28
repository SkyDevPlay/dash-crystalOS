#include "sys/pic.h"
#include "sys/ps2.h"
#include "sys/ports.h"
#include "sys/keymap.h"
#include "sys/io.h"
#include "sys/lba.h"
#include "malloc.h"
#include "io.h"
#include "mbr.h"
#include "fat.h"
#include "shell.h"

struct mbr *mbr = (void *)0x7c00;

#define KB_DATA    0x60
#define KB_COMMAND 0x64

static int is_shift = 0;

void handle_keyboard(void) {
    u8 status = inb(KB_COMMAND);
    if (!(status & 1)) goto eoi;

    u8 keycode = inb(KB_DATA);

    if      (keycode == LSHIFT)        is_shift = 1;
    else if (keycode == LSHIFT + 0x80) is_shift = 0;
    else if (keycode == LMAJ)          is_shift = !is_shift;
    else
        shell_handle_key(keycode, is_shift);

eoi:
    outb(PIC1_COMMAND, 0x20);
}

int main(void) {

    init_idt();
    kb_init();
    enable_interrupts();

    u16 low_memory      = 1024;
    u16 upper_memory    = *((u16 *)0x802);
    u16 extended_memory = *((u16 *)0x804);
    u32 available_memory = 1024u * (low_memory + upper_memory + extended_memory * 64u);
    printf("Memory available : %d Ko\n", available_memory / 1024);

    if (init_malloc(available_memory) < 0) {
        setColor(RED);
        puts("ERROR: init_malloc crashed");
        return 0;
    }

    if (initSerial(coms[0])) {
        printf("Serial port failed to initialize\n");
    }

    printf("lba_start = %d\n", mbr->parts[0].lba_start);

    if (init_fs(mbr->parts[0].lba_start) < 0) {
        setColor(RED);
        printf("WARNING FATsystem unmounted(lba=%d)\n", mbr->parts[0].lba_start);
        setColor(WHITE);
    }

    shell_init();

    for (;;) {
        __asm__("hlt");
    }

    return 0;
}

