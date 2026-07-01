#include "sys/paging.h"

u32 page_directory[1024]__attribute__((aligned(4096)));
u32 page_tables[1024][1024]__attribute__((aligned(4096)));

void init_paging(u32 available_memory) {
    u32 num_tables = (available_memory + 4194304 - 1) / 4194304;
    for (int j = 0; j < num_tables; j++){
        for (int i = 0; i < 1024; i++) {
            page_tables[j][i] = ((j * 1024 + i) * 4096) | PAGE_PRESENT | PAGE_RW;
        }
        page_directory[j] = ((u32)page_tables[j]) | PAGE_PRESENT | PAGE_RW;
    }
    for (int j = num_tables; j < 1024; j++) {
        page_directory[j] = 0;
    }

}

void enable_paging(void) {
    asm volatile("mov %0, %%cr3" :: "r"(page_directory));
    u32 cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= (1 << 31);
    asm volatile("mov %0, %%cr0" :: "r"(cr0));
}