#include "sys/paging.h"

u32 page_directory[1024]__attribute__((aligned(4096)));
u32 page_table[NUM_PAGE_TABLES][1024]__attribute__((aligned(4096)));

void init_paging(void) {
    for (int t = 0; t < NUM_PAGE_TABLES; t++) {
        for (int i = 0; i < 1024; i++) {
            u32 addr = (t * 1024 + i) * 4096;
            page_table[t][i] = addr | PAGE_PRESENT | PAGE_RW;
        }
        page_directory[t] = ((u32)page_table[t]) | PAGE_PRESENT | PAGE_RW;
    }

    for (int i = NUM_PAGE_TABLES; i < 1024; i++) {
        page_directory[i] = 0;
    }

}

void enable_paging(void) {
    asm volatile("mov %0, %%cr3" :: "r"(page_directory));
    u32 cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= (1 << 31);
    asm volatile("mov %0, %%cr0" :: "r"(cr0));
}