#ifndef PAGING_H
#define PAGING_H

#include "types.h"

#define PAGE_PRESENT (1 << 0)
#define PAGE_RW (1 << 1)
#define PAGE_USER (1 << 2)

extern u32 page_directory[1024]__attribute__((aligned(4096)));
extern u32 page_tables[1024][1024]__attribute__((aligned(4096)));

void init_paging(u32 available_memory);
void enable_paging(void);

#endif
