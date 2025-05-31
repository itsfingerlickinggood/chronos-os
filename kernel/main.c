#include <stdio.h>
#include "kernel/scheduler.h"
#include "kernel/memory.h"
#include "kernel/ai_core.h"

int kmain() {
    memory_init(); // Initialize the memory manager
    printf("Kernel main initialized. Memory manager started.\n");

    run_memory_tests(); // Run memory manager tests

    printf("Memory tests complete. Halting or proceeding...\n");
    return 0;
}
