#ifndef KERNEL_DASHBOARD_H
#define KERNEL_DASHBOARD_H

#include <stdint.h>

#include "kernel/scheduler.h"

typedef struct dashboard_metrics {
    uint32_t pit_frequency;
    uint64_t ticks;
    uint32_t uptime_seconds;
    pid_t current_pid;
    uint32_t running_tasks;
    uint32_t ready_tasks;
    uint32_t waiting_tasks;
    uint32_t sleeping_tasks;
    uint32_t terminated_tasks;
    uint32_t total_tasks;
    uint32_t dispatch_count;
    uint32_t completed_tasks;
    uint32_t created_tasks;
} dashboard_metrics_t;

void dashboard_init(uint32_t pit_frequency);
void dashboard_update(const dashboard_metrics_t* metrics);

#endif // KERNEL_DASHBOARD_H
