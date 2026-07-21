#ifndef TASK_H
#define TASK_H

#include "../types.h"

typedef void (*task_entry_t)(void);

void task_init(void);
int task_create(task_entry_t entry, const char *name);
u32 *task_schedule(u32 *current_stack);
int task_count(void);
void task_list(void);

#endif
