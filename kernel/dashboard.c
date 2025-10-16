#include "kernel/dashboard.h"

#include "kernel/printf.h"

void dashboard_init(uint32_t pit_frequency) {
    kprintf("\n=== Kernel Dashboard ===\n");
    kprintf("Timer frequency: %d Hz\n", (int)pit_frequency);
}

void dashboard_update(const dashboard_metrics_t* metrics) {
    if (!metrics) {
        return;
    }

    int uptime_seconds = (int)metrics->uptime_seconds;
    int current_ticks = (int)(metrics->ticks & 0xFFFFFFFFu);
    int current_pid = metrics->current_pid;
    int active_tasks = (int)metrics->total_tasks;
    int running = (int)metrics->running_tasks;
    int ready = (int)metrics->ready_tasks;
    int waiting = (int)metrics->waiting_tasks;
    int sleeping = (int)metrics->sleeping_tasks;
    int terminated = (int)metrics->terminated_tasks;
    int dispatch_count = (int)metrics->dispatch_count;
    int completed = (int)metrics->completed_tasks;
    int created = (int)metrics->created_tasks;

    kprintf("[dashboard] uptime=%ds ticks=%d | current=%d | active=%d (run=%d ready=%d wait=%d sleep=%d term=%d) | created=%d completed=%d | dispatches=%d\n",
            uptime_seconds,
            current_ticks,
            current_pid,
            active_tasks,
            running,
            ready,
            waiting,
            sleeping,
            terminated,
            created,
            completed,
            dispatch_count);
}
