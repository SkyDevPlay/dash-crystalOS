#include "sys/task.h"
#include "malloc.h"
#include "io.h"
#include "string.h"

#define MAX_TASKS 8
#define TASK_STACK_SIZE 4096

enum task_state { TASK_UNUSED, TASK_READY, TASK_RUNNING, TASK_STOPPED };

struct task {
    u32 *stack;
    void *stack_base;
    task_entry_t entry;
    char name[16];
    enum task_state state;
};

static struct task tasks[MAX_TASKS];
static int current_task;

static void task_bootstrap(void);

static u32 irq_save_disable(void) {
    u32 flags;
    __asm__ volatile("pushf; pop %0; cli" : "=r"(flags) :: "memory");
    return flags;
}

static void irq_restore(u32 flags) {
    if (flags & (1 << 9)) __asm__ volatile("sti" ::: "memory");
}

void task_init(void) {
    memset(tasks, 0, sizeof(tasks));
    tasks[0].state = TASK_RUNNING;
    strncpy(tasks[0].name, "kernel", sizeof(tasks[0].name) - 1);
    current_task = 0;
}

int task_create(task_entry_t entry, const char *name) {
    if (!entry) return -1;
    u32 flags = irq_save_disable();

    int slot = -1;
    for (int i = 1; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_UNUSED || tasks[i].state == TASK_STOPPED) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        irq_restore(flags);
        return -1;
    }

    void *stack_base = malloc(TASK_STACK_SIZE);
    if (!stack_base) {
        irq_restore(flags);
        return -1;
    }

    u32 *sp = (u32 *)((u8 *)stack_base + TASK_STACK_SIZE);
    *--sp = 0x202;                 /* EFLAGS: interrupts enabled. */
    *--sp = 0x08;                  /* Kernel code selector. */
    *--sp = (u32)task_bootstrap;   /* EIP restored by iretd. */
    for (int i = 0; i < 8; i++) *--sp = 0; /* Registers restored by popad. */

    memset(&tasks[slot], 0, sizeof(tasks[slot]));
    tasks[slot].stack = sp;
    tasks[slot].stack_base = stack_base;
    tasks[slot].entry = entry;
    tasks[slot].state = TASK_READY;
    if (name) strncpy(tasks[slot].name, (char *)name, sizeof(tasks[slot].name) - 1);
    else strncpy(tasks[slot].name, "task", sizeof(tasks[slot].name) - 1);
    irq_restore(flags);
    return slot;
}

u32 *task_schedule(u32 *current_stack) {
    if (tasks[current_task].state == TASK_RUNNING)
        tasks[current_task].stack = current_stack;

    for (int offset = 1; offset < MAX_TASKS; offset++) {
        int next = (current_task + offset) % MAX_TASKS;
        if (tasks[next].state != TASK_READY) continue;

        if (tasks[current_task].state == TASK_RUNNING)
            tasks[current_task].state = TASK_READY;
        tasks[next].state = TASK_RUNNING;
        current_task = next;
        return tasks[next].stack;
    }
    return current_stack;
}

static void task_bootstrap(void) {
    task_entry_t entry = tasks[current_task].entry;
    entry();
    tasks[current_task].state = TASK_STOPPED;
    for (;;) __asm__("hlt");
}

int task_count(void) {
    int count = 0;
    for (int i = 0; i < MAX_TASKS; i++)
        if (tasks[i].state == TASK_READY || tasks[i].state == TASK_RUNNING) count++;
    return count;
}

void task_list(void) {
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state != TASK_READY && tasks[i].state != TASK_RUNNING) continue;
        printf("%d  %s  %s\n", i, tasks[i].name,
               tasks[i].state == TASK_RUNNING ? "running" : "ready");
    }
}
