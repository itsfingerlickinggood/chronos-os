#include "kernel/vga.h"       // For vga_init
#include "kernel/printf.h"    // For kprintf
#include "kernel/scheduler.h" // Includes pcb_t, task_state, create_task, schedule, current_task, tasks, MAX_TASKS
#include "kernel/memory.h"    // Includes kmalloc, kfree, memory_init
#include "kernel/ai_core.h"   // Not used in this part, but kept

// --- Dummy Task Functions ---
// These functions will run to completion when called.
// The scheduler will switch between them based on the kmain loop.

void task1_func(void) {
    pid_t my_id = -1;
    if (current_task && current_task->instruction_pointer == (uintptr_t)task1_func) {
        my_id = current_task->id;
    }

    for (int i = 0; i < 3; ++i) { // Reduced cycles for quicker demo
        kprintf("Task 1 (ID: %d) executing cycle %d\n", my_id, i);
        // Simulate some work by delaying
        for (volatile int j = 0; j < 1000000; ++j);
        // In a cooperative model, a task would yield here.
        // Our kmain loop calls schedule() externally.
    }
    kprintf("Task 1 (ID: %d) COMPLETED\n", my_id);
    if (current_task && current_task->id == my_id) {
        current_task->state = TASK_TERMINATED;
    }
}

void task2_func(void) {
    pid_t my_id = -1;
    if (current_task && current_task->instruction_pointer == (uintptr_t)task2_func) {
        my_id = current_task->id;
    }

    for (int i = 0; i < 3; ++i) { // Reduced cycles
        kprintf("Task 2 (ID: %d) executing cycle %d\n", my_id, i);
        for (volatile int j = 0; j < 800000; ++j);
    }
    kprintf("Task 2 (ID: %d) COMPLETED\n", my_id);
    if (current_task && current_task->id == my_id) {
        current_task->state = TASK_TERMINATED;
    }
}

void task3_func(void) {
    pid_t my_id = -1;
    if (current_task && current_task->instruction_pointer == (uintptr_t)task3_func) {
        my_id = current_task->id;
    }
    kprintf("Task 3 (ID: %d) executing its single action and completing.\n", my_id);
    for (volatile int j = 0; j < 500000; ++j);
    kprintf("Task 3 (ID: %d) COMPLETED\n", my_id);
    if (current_task && current_task->id == my_id) {
        current_task->state = TASK_TERMINATED;
    }
}


// --- Kernel Main ---
int kmain() {
    vga_init(); // Initialize VGA display first
    kprintf("Kernel main: VGA initialized.\n");

    memory_init(); // Initialize the memory manager
    kprintf("Kernel main: Memory manager started.\n");

    task_init_system(); // Initialize the scheduler system
    kprintf("Kernel main: Scheduler system initialized.\n");

    // Run memory tests (optional, can be commented out for scheduler focus)
    // run_memory_tests(); // This would need kprintf internally too
    // kprintf("Kernel main: Memory tests complete.\n");

    kprintf("\nkmain: Creating tasks...\n");
    pid_t pid1 = create_task(task1_func, 0); // priority 0
    if (pid1 > 0) {
        kprintf("kmain: Created task1_func with PID %d\n", pid1);
    } else {
        kprintf("kmain: Failed to create task1_func (error %d)\n", pid1);
    }

    pid_t pid2 = create_task(task2_func, 0); // priority 0
    if (pid2 > 0) {
        kprintf("kmain: Created task2_func with PID %d\n", pid2);
    } else {
        kprintf("kmain: Failed to create task2_func (error %d)\n", pid2);
    }

    pid_t pid3 = create_task(task3_func, 0); // priority 0
    if (pid3 > 0) {
        kprintf("kmain: Created task3_func with PID %d\n", pid3);
    } else {
        kprintf("kmain: Failed to create task3_func (error %d)\n", pid3);
    }

    kprintf("\nkmain: Starting simulated cooperative scheduling loop...\n");
    // This loop simulates a timer interrupt or cooperative yields calling schedule().
    // The tasks themselves run to completion when their function is "called".
    // This is a high-level simulation of the scheduler's decision-making.

    for (int i = 0; i < 20; ++i) { // Simulate for a number of scheduler "ticks"
        schedule(); // Decide who runs next

        if (current_task != NULL) {
            kprintf("kmain [Tick %2d]: Scheduled Task ID: %d, State: %d, IP: %p\n",
                   i, current_task->id, current_task->state, (void*)current_task->instruction_pointer);

            // SIMULATE TASK EXECUTION FOR ONE "SLICE"
            // In a real kernel, a context switch would occur. Here, we directly call
            // the task's function if it's its "turn" and it hasn't run yet,
            // or conceptually allow it to "continue".
            // This is highly simplified. The dummy tasks run to completion once called.
            // For this test, we'll call the function associated with current_task->instruction_pointer
            // This means tasks will run to completion when they are first scheduled.
            if (current_task->state == TASK_RUNNING) {
                 // This is a conceptual representation. The actual task function is not
                 // being re-entered or managed with true context switching here.
                 // The task function is called ONCE when the task is first scheduled.
                 // If the task function had its own loop and yielded, this would be different.
                 // Our dummy tasks complete and set their state to TERMINATED.

                 // Let's find the function pointer from the PCB and call it.
                 // This is a very basic way to simulate dispatch for tasks that run to completion.
                 void (*task_function)(void) = (void (*)(void))current_task->instruction_pointer;

                 // Check if this task was already started.
                 // This simple check is not robust for re-entrant tasks or true preemption.
                 // We'll assume tasks run to completion on their "first call".
                 // The `schedule()` function already handles re-queueing.
                 // The key is that `current_task` has been set by `schedule()`.
                 // If we call task_function() here every time, and tasks have loops,
                 // they will restart. We need a mechanism to know if a task is "running for the first time"
                 // or "resuming". Our current setup doesn't have true resume.
                 // The tasks run to completion and self-terminate.

                 // The following direct call simulates dispatching to the task.
                 // It will run to completion.
                 kprintf("kmain [Tick %2d]: Dispatching to Task ID: %d...\n", i, current_task->id);
                 task_function(); // Execute the current task's function
                 kprintf("kmain [Tick %2d]: Task ID: %d returned from execution.\n", current_task->id, i); // Corrected order of args
            }


            // Check if the task terminated itself during its run
            if (current_task->state == TASK_TERMINATED) {
                kprintf("kmain [Tick %2d]: Task ID %d has self-terminated. Cleaning up.\n", i, current_task->id);
                kfree(current_task->stack_base); // Free its stack
                current_task->stack_base = NULL;
                current_task->state = TASK_UNUSED;   // Mark PCB as unused
                // current_task will be NULL in the next iteration if schedule() doesn't find another
            }
        } else {
            kprintf("kmain [Tick %2d]: No task scheduled (CPU Idle).\n", i);
            // Check if all tasks are done
            int all_done = 1;
            for (int task_idx = 0; task_idx < MAX_TASKS; ++task_idx) {
                if (tasks[task_idx].state != TASK_UNUSED && tasks[task_idx].id != 0) {
                    all_done = 0;
                    break;
                }
            }
            if (all_done) {
                kprintf("kmain: All tasks are unused. Ending simulation.\n");
                break;
            }
        }

        // Simulate a delay for the scheduler tick
        for (volatile int k = 0; k < 100000; ++k); // Shorter delay for faster simulation
    }

    kprintf("\nkmain: Simulated scheduling loop finished.\n");
    kprintf("Final PCB States:\n");
    for (int i = 0; i < MAX_TASKS; ++i) {
        if (tasks[i].id != 0) { // Print only potentially used tasks
            kprintf("  PCB %d: PID %d, State %d, Stack %p\n",
                   i, tasks[i].id, tasks[i].state, tasks[i].stack_base);
        }
    }

    return 0;
}
