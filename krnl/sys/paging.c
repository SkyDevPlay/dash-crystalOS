#include "sys/paging.h"

u32 page_directory[1024]__attribute__((aligned(4096)));
u32 page_table[1024]__attribute__((aligned(4096)));

void init_paging(void) {
    for (int i = 0; i < 1024; i++) {
        page_table[i] = (i * 4096) | PAGE_PRESENT | PAGE_RW;
    }

    page_directory[0] = ((u32)page_table) | PAGE_PRESENT | PAGE_RW;

    for (int i = 1; i < 1024; i++) {
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