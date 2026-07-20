#include "sys/timer.h"
#include "sys/ports.h"
#include "sys/pic.h"

static volatile u32 ticks = 0;
static u32 timer_frequency = 100; // default 100Hz

void init_timer(u32 frequency) {
    if (frequency == 0) frequency = 100;
    if (frequency > 1193182) frequency = 1193182;
    timer_frequency = frequency;
    u32 divisor = 1193182 / frequency;

    // Send the command byte (0x36) to the PIT Control Port (0x43)
    // 0x36 = 0b00110110:
    // - Bits 6-7: 00 (Channel 0)
    // - Bits 4-5: 11 (Access mode: lobyte/hibyte)
    // - Bits 1-3: 011 (Operating mode: square wave generator)
    // - Bit 0: 0 (Binary mode)
    outb(0x43, 0x36);

    // Divisor must be sent byte-by-byte to PIT Channel 0 Data Port (0x40)
    u8 low  = (u8)(divisor & 0xFF);
    u8 high = (u8)((divisor >> 8) & 0xFF);

    outb(0x40, low);
    outb(0x40, high);
}

void handle_timer(void) {
    ticks++;
    // Send End of Interrupt (EOI) to PIC1
    outb(PIC1_COMMAND, 0x20);
}

u32 get_ticks(void) {
    return ticks;
}

void sleep(u32 ms) {
    u32 ticks_to_wait = (ms * timer_frequency) / 1000;
    if (ticks_to_wait == 0 && ms > 0) {
        ticks_to_wait = 1;
    }
    u32 start = ticks;
    while (ticks - start < ticks_to_wait) {
        __asm__("hlt");
    }
}
