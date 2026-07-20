#ifndef TIMER_H
#define TIMER_H

#include "../types.h"

void init_timer(u32 frequency);
void sleep(u32 ms);
u32 get_ticks(void);

#endif
