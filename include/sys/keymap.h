#ifndef KEYMAP_H
#define KEYMAP_H

typedef enum {
    LSHIFT = 0x2A,
    LMAJ = 0x3A,
    BACKSPACE = 0x0E,
    ENTER = 0x1C
} Keys;

extern char keymap[128];
extern char keymap_maj[128];
#endif
