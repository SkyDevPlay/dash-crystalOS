#include "paging.h"

u32 page_directory[1024]_attribute_((aligned(4096)));
u32 page_table[1024]_attribute_((aligned(4096)));