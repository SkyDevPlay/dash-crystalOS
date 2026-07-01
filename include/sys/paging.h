#ifndef PAGING_H
#define PAGING_H

#include "types.h"

#define PAGE_PRESENT (1 << 0)
#define PAGE_RW (1 << 1)
#define PAGE_USER (1 << 2)

extern u32 page_directory[1024]_attribute_((aligned(4096)));
extern u32 page_table[1024]_attribute_((aligned(4096)));

#endif
