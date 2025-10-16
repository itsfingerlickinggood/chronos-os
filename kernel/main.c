#include <stdint.h>

#include "kernel/vga.h"
#include "kernel/printf.h"
#include "kernel/scheduler.h"
#include "kernel/memory.h"
#include "kernel/dashboard.h"
#include "arch/x86/idt.h"
#include "arch/x86/pic.h"
#include "arch/x86/timer.h"

static void simulate_workload(uint32_t iterations) {
    for (volatile uint32_t i = 0; i < iterations; ++i) {
    }
}

static void complete_current_task(void) {
    if (current_task) {
        current_task->state = TASK_TERMINATED;
    }
}

static void task1_func(void) {
    simulate_workload(250000);
    complete_current_task();
}

static void task2_func(void) {
    simulate_workload(400000);
    complete_current_task();
}

static void task3_func(void) {
    simulate_workload(150000);
    complete_current_task();
}

static void dispatch_task(pcb_t* task) {
    if (!task || task->state != TASK_RUNNING) {
        return;
    }

    void (*task_fn)(void) = (void (*)(void))task->instruction_pointer;
    if (task_fn) {
        task_fn();
    }
}

static void cleanup_terminated_task(pcb_t* task) {
    if (!task || task->state != TASK_TERMINATED) {
        return;
    }

    if (task->stack_base) {
        kfree(task->stack_base);
    }

    task->stack_base = NULL;
    task->stack_pointer = 0;
    task->instruction_pointer = 0;
    task->priority = 0;
    task->next = NULL;
    task->id = 0;
    task->state = TASK_UNUSED;
}

static void collect_dashboard_metrics(dashboard_metrics_t* metrics,
                                      uint32_t pit_frequency,
                                      uint64_t ticks,
                                      uint32_t dispatch_count,
                                      uint32_t completed_tasks,
                                      uint32_t created_tasks) {
    if (!metrics) {
        return;
    }

    metrics->pit_frequency = pit_frequency;
    metrics->ticks = ticks;
    metrics->uptime_seconds = (pit_frequency > 0) ? (uint32_t)(ticks / pit_frequency) : 0;
    metrics->dispatch_count = dispatch_count;
    metrics->completed_tasks = completed_tasks;
    metrics->created_tasks = created_tasks;
    metrics->current_pid = (current_task && current_task->state == TASK_RUNNING) ? current_task->id : -1;

    metrics->running_tasks = 0;
    metrics->ready_tasks = 0;
    metrics->waiting_tasks = 0;
    metrics->sleeping_tasks = 0;
    metrics->terminated_tasks = 0;
    metrics->total_tasks = 0;

    for (int i = 0; i < MAX_TASKS; ++i) {
        enum task_state state = tasks[i].state;
        if (state == TASK_UNUSED) {
            continue;
        }

        metrics->total_tasks++;
        switch (state) {
            case TASK_RUNNING:
                metrics->running_tasks++;
                break;
            case TASK_READY:
                metrics->ready_tasks++;
                break;
            case TASK_WAITING:
                metrics->waiting_tasks++;
                break;
            case TASK_SLEEPING:
                metrics->sleeping_tasks++;
                break;
            case TASK_TERMINATED:
                metrics->terminated_tasks++;
                break;
            default:
                break;
        }
    }
}

int kmain(void) {
    vga_init();

    idt_init();
    pic_remap(0x20, 0x28);

    uint32_t timer_frequency = 100;
    pit_init(timer_frequency);

    irq_clear_mask(0);
    asm volatile("sti");

    memory_init();
    task_init_system();

    uint32_t created_tasks = 0;
    if (create_task(task1_func, 0) > 0) {
        created_tasks++;
    }
    if (create_task(task2_func, 0) > 0) {
        created_tasks++;
    }
    if (create_task(task3_func, 0) > 0) {
        created_tasks++;
    }

    dashboard_init(timer_frequency);

    dashboard_metrics_t metrics;
    uint32_t dispatch_count = 0;
    uint32_t completed_tasks = 0;

    uint64_t ticks = get_system_ticks();
    collect_dashboard_metrics(&metrics, timer_frequency, ticks, dispatch_count, completed_tasks, created_tasks);
    dashboard_update(&metrics);

    uint64_t last_dashboard_ticks = ticks;
    uint32_t dashboard_interval = (timer_frequency > 0) ? timer_frequency : 1;

    while (1) {
        schedule();

        pcb_t* running = current_task;

        if (running && running->state == TASK_RUNNING) {
            dispatch_task(running);
            dispatch_count++;
        }

        if (running && running->state == TASK_TERMINATED) {
            cleanup_terminated_task(running);
            current_task = NULL;
            completed_tasks++;
        }

        ticks = get_system_ticks();
        if ((ticks - last_dashboard_ticks) >= dashboard_interval) {
            collect_dashboard_metrics(&metrics, timer_frequency, ticks, dispatch_count, completed_tasks, created_tasks);
            dashboard_update(&metrics);
            last_dashboard_ticks = ticks;
        }

        asm volatile("hlt");
    }

    return 0;
}
