#include "kernel/scheduler.h"
#include "kernel/memory.h" // For kmalloc/kfree for task stacks (used in create_task)
#include <stddef.h>       // For NULL
#include <stdio.h>        // For printf (temporary for debugging)

// --- Static global variables for task management ---
#define MAX_TASKS 32 // Maximum number of tasks the system can handle

static pcb_t tasks[MAX_TASKS];         // Array of PCBs
static pcb_t* current_task = NULL;    // Pointer to the currently executing task
static pcb_t* ready_queue_head = NULL; // Head of the ready task queue (singly linked list)
static pid_t next_pid = 1;             // Counter for assigning new PIDs

// --- Scheduler Initialization ---

/**
 * @brief Initializes the task management system.
 *
 * Clears the PCB table, sets up initial scheduler state.
 * This function should be called once during kernel initialization.
 * Corresponds to task_init_system in scheduler.h
 */
void task_init_system(void) {
    for (int i = 0; i < MAX_TASKS; ++i) {
        tasks[i].id = 0; // PID 0 is often reserved or indicates invalid/unused
        tasks[i].state = TASK_UNUSED;
        tasks[i].stack_pointer = 0;
        tasks[i].instruction_pointer = 0;
        tasks[i].stack_base = NULL;
        tasks[i].next = NULL;
    }
    current_task = NULL;
    ready_queue_head = NULL;
    next_pid = 1; // Start PIDs from 1
    printf("Scheduler initialized. Max tasks: %d\n", MAX_TASKS);
}

// --- Ready Queue Management ---

/**
 * @brief Adds a task to the end of the ready queue.
 *
 * Sets the task's state to TASK_READY.
 * @param task Pointer to the PCB of the task to enqueue.
 */
static void enqueue_task(pcb_t* task) {
    if (!task) {
        printf("enqueue_task: Attempted to enqueue a NULL task.\n");
        return;
    }
    if (task->state != TASK_UNUSED && task->state != TASK_TERMINATED && task->state != TASK_WAITING && task_state != TASK_SLEEPING) {
        // Only tasks that are not already in a runnable/ready state should be enqueued directly.
        // Or tasks that are being moved from waiting/sleeping to ready.
        // For simplicity here, we assume it's a new or unblocked task.
        // A more robust check might be needed depending on how tasks are created/managed.
         printf("enqueue_task: Task %d is not in a state to be enqueued (state: %d).\n", task->id, task->state);
        // return; // For now, let's allow re-enqueuing for flexibility, but log it.
    }

    task->state = TASK_READY;
    task->next = NULL; // Ensure it's the new tail

    if (ready_queue_head == NULL) {
        ready_queue_head = task;
    } else {
        pcb_t* current = ready_queue_head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = task;
    }
    // printf("Task %d enqueued. Ready queue head: %p\n", task->id, ready_queue_head);
}

/**
 * @brief Removes and returns the task from the head of the ready queue.
 *
 * @return pcb_t* Pointer to the dequeued task, or NULL if the queue is empty.
 */
static pcb_t* dequeue_task(void) {
    if (ready_queue_head == NULL) {
        return NULL;
    }
    pcb_t* task = ready_queue_head;
    ready_queue_head = ready_queue_head->next;
    task->next = NULL; // Detach from the list before returning
    // printf("Task %d dequeued. New ready queue head: %p\n", task->id, ready_queue_head);
    return task;
}

// --- Placeholder Implementations for Scheduler Functions (from scheduler.h) ---

/**
 * @brief The main scheduler function.
 *
 * (To be implemented)
 * This function will select the next task to run from the ready queue,
 * perform a context switch if necessary.
 * For now, it's a placeholder.
 */
void schedule(void) {
    // --- Save context of current_task (conceptual for now) ---
    // In a real preemptive scheduler, this is where you'd save CPU registers,
    // stack pointer, instruction pointer of current_task.
    // For a cooperative scheduler, the task is assumed to have saved its own state
    // or is at a point where it can be safely "paused".

    pcb_t* prev_task = current_task;

    // --- Handle the previous task (if any) ---
    if (prev_task != NULL) {
        if (prev_task->state == TASK_RUNNING) {
            // If the task was running, it's now just ready. Enqueue it.
            // This makes it a round-robin scheduler.
            prev_task->state = TASK_READY;
            enqueue_task(prev_task);
            // printf("schedule: Task %d (was running) enqueued, state set to READY.\n", prev_task->id);
        } else if (prev_task->state == TASK_TERMINATED) {
            // If the task terminated itself (or was marked for termination):
            // - Its stack should be freed by terminate_task().
            // - Its PCB should be marked TASK_UNUSED by terminate_task().
            // For now, we just acknowledge it and don't re-enqueue.
            // A call to terminate_task should have handled cleanup.
             printf("schedule: Task %d was TERMINATED. It will not be re-enqueued.\n", prev_task->id);
        }
        // If prev_task->state is TASK_WAITING or TASK_SLEEPING, it means the task blocked itself.
        // It should have been removed from the ready queue by block_task() or similar.
        // So, we don't enqueue it here. It will be moved back to ready by unblock_task()
        // or a timer handler waking it up.
        // else if (prev_task->state == TASK_WAITING || prev_task->state == TASK_SLEEPING) {
        //    printf("schedule: Task %d is WAITING/SLEEPING. Not re-enqueued.\n", prev_task->id);
        // }
    }

    // --- Select next task ---
    pcb_t* next_task = dequeue_task();

    if (next_task != NULL) {
        // A task is available in the ready queue.
        current_task = next_task;
        current_task->state = TASK_RUNNING;
        // printf("schedule: Switching to task %d (idx %ld). State set to RUNNING.\n",
        //        current_task->id, (current_task - tasks));
    } else if (prev_task != NULL && prev_task->state == TASK_READY) {
        // This case handles if the ready queue was empty, but the `prev_task` was
        // re-enqueued and is the only task ready. So, it continues running.
        current_task = prev_task; // It was just enqueued, now dequeue it effectively by making it current
        ready_queue_head = NULL; // Since it was the only one and we just took it
        current_task->state = TASK_RUNNING;
        // printf("schedule: No other tasks. Continuing with task %d (idx %ld).\n",
        //        current_task->id, (current_task - tasks));
    } else if (prev_task != NULL && (prev_task->state == TASK_WAITING || prev_task->state == TASK_SLEEPING)) {
        // The previous task is blocked, and no other task is ready.
        // The system should ideally enter an idle state or wait for an interrupt.
        current_task = NULL; // Or set to a specific idle task.
        // printf("schedule: Previous task %d is WAITING/SLEEPING and no other tasks ready. Idling.\n", prev_task->id);
    }
     else {
        // No task was running (or it terminated and wasn't re-enqueued),
        // and the ready queue is empty.
        current_task = NULL; // No task to run.
        // printf("schedule: No tasks to run. CPU Idle.\n");
    }

    // --- Restore context of current_task (conceptual for now) ---
    // If current_task is not NULL, this is where you'd load its saved registers,
    // set the stack pointer, and jump to its saved instruction pointer.
    // This is the actual context switch.
    // if (current_task != NULL) {
    //    printf("schedule: Task %d is now current_task. (Conceptual run at %p)\n",
    //           current_task->id, (void*)current_task->instruction_pointer);
    // } else if (prev_task && prev_task->state != TASK_TERMINATED) {
    //    // If there's no current task, but the previous task is waiting/sleeping, it remains "current" in some sense
    //    // until an interrupt or event changes its state.
    //    current_task = prev_task;
    //    printf("schedule: No runnable task, current_task remains %d (state: %d)\n", current_task->id, current_task->state);
    // }


    // The actual execution of `current_task` happens after `schedule()` returns,
    // typically by jumping to `current_task->instruction_pointer`.
    // In a cooperative model, the task itself will call a "yield" function which calls schedule.
    // In a preemptive model, an interrupt handler (e.g., timer) calls schedule.
}

/**
 * @brief Creates a new task.
 *
 * (To be implemented)
 * - Find an unused PCB.
 * - Allocate a stack for the task using kmalloc.
 * - Initialize the stack for the task to start at entry_point.
 * - Set PCB fields (PID, state, stack_pointer, instruction_pointer, stack_base).
 * - Enqueue the task in the ready queue.
 * @param entry_point Function pointer to where the new task should begin execution.
 * @param priority Task priority (currently unused, for future extension).
 * @return pid_t The PID of the created task, or a negative error code on failure.
 *               (-1: No unused PCBs, -2: Stack allocation failed)
 */
pid_t create_task(void (*entry_point)(void), int priority) {
    pcb_t* new_task_pcb = NULL;
    int new_task_idx = -1;

    // --- 1. Find an unused PCB ---
    // This section should be atomic if preemption is possible.
    // For now, assuming no preemption during this critical section.
    for (int i = 0; i < MAX_TASKS; ++i) {
        if (tasks[i].state == TASK_UNUSED) {
            new_task_pcb = &tasks[i];
            new_task_idx = i;
            break;
        }
    }

    if (new_task_pcb == NULL) {
        printf("create_task: No unused PCBs available!\n");
        return -1; // Error code for no free PCBs
    }

    // --- 2. Allocate a stack for the new task ---
    // KERNEL_STACK_SIZE is defined in include/kernel/scheduler.h
    void* stack_bottom = kmalloc(KERNEL_STACK_SIZE);
    if (stack_bottom == NULL) {
        printf("create_task: Failed to allocate stack for new task (potential PID %d)!\n", next_pid);
        // Important: Do not mark the PCB as used if stack allocation fails.
        return -2; // Error code for stack allocation failure
    }

    // --- 3. Initialize PCB fields ---
    new_task_pcb->id = next_pid++; // Assign and increment the global PID counter
    new_task_pcb->stack_base = stack_bottom;
    new_task_pcb->priority = priority; // Set the task's priority

    // 4. Initialize the task's context (instruction pointer and stack pointer)
    // This is a highly simplified setup for a non-preemptive or cooperative scheduler
    // where context switching isn't fully implemented yet.
    // The instruction_pointer will point to the function the task should execute.
    new_task_pcb->instruction_pointer = (uintptr_t)entry_point;
    // The stack pointer is set to the top of the allocated stack.
    // Stacks typically grow downwards.
    // Subtracting sizeof(uintptr_t) makes space for one item, or just sets it to the very end.
    new_task_pcb->stack_pointer = (uintptr_t)stack_bottom + KERNEL_STACK_SIZE - sizeof(uintptr_t);

    // In a real kernel, you would initialize the stack with a "context frame" here.
    // This frame would contain initial values for CPU registers (EAX, EBX, etc.),
    // segment registers (CS, DS, etc.), flags (EFLAGS), and the entry_point (EIP).
    // This allows the context switcher to simply "restore" this frame to start the task.
    // For example (conceptual for x86):
    // uintptr_t *sp = (uintptr_t*)new_task_pcb->stack_pointer;
    // *(--sp) = initial_eflags;  // EFLAGS
    // *(--sp) = initial_cs;      // CS (Code Segment)
    // *(--sp) = (uintptr_t)entry_point; // EIP (Instruction Pointer)
    // // Push initial general-purpose registers ( GPRs values, often 0 or specific args)
    // *(--sp) = 0; // EAX
    // *(--sp) = 0; // EBX
    // ... and so on for EDI, ESI, EBP, ESP (original value before pushes), EDX, ECX.
    // new_task_pcb->stack_pointer = (uintptr_t)sp; // Update SP to new top

    // --- 5. Add the new task to the ready queue ---
    // The enqueue_task function will set the task's state to TASK_READY.
    enqueue_task(new_task_pcb);

    printf("create_task: PID %d (PCB idx %d) created. Entry: %p, Stack: [%p-%p], Priority: %d\n",
           new_task_pcb->id, new_task_idx, (void*)new_task_pcb->instruction_pointer,
           new_task_pcb->stack_base, (void*)( (char*)new_task_pcb->stack_base + KERNEL_STACK_SIZE -1 ), new_task_pcb->priority);

    return new_task_pcb->id; // Return the new Process ID
}

/**
 * @brief Terminates the specified task.
 *
 * (To be implemented)
 * - Mark PCB as TASK_TERMINATED or TASK_UNUSED.
 * - Free its stack using kfree.
 * - Remove from any queues.
 * - If it's the current_task, call schedule() to pick a new one.
 * @param id PID of the task to terminate.
 */
void terminate_task(pid_t id) {
    // TODO: Implement task termination
    (void)id; // Suppress unused parameter warning
    printf("terminate_task() called for PID %d. (Not yet implemented)\n", id);
}

/**
 * @brief Blocks the specified task.
 *
 * (To be implemented)
 * - Set task state to the given reason (e.g., TASK_WAITING, TASK_SLEEPING).
 * - Remove from ready queue (if it was there).
 * - Add to a wait queue if applicable.
 * - If it's the current_task, call schedule().
 * @param id PID of the task to block.
 * @param reason The state to set the task to (e.g., TASK_WAITING).
 */
void block_task(pid_t id, enum task_state reason) {
    // TODO: Implement task blocking
    (void)id;     // Suppress unused parameter warning
    (void)reason; // Suppress unused parameter warning
    printf("block_task() called for PID %d with reason %d. (Not yet implemented)\n", id, reason);
}

/**
 * @brief Unblocks a task and moves it to the ready queue.
 *
 * (To be implemented)
 * - Find PCB by PID.
 * - Set state to TASK_READY.
 * - Remove from wait queue (if any).
 * - Enqueue into ready queue.
 * @param id PID of the task to unblock.
 */
void unblock_task(pid_t id) {
    // TODO: Implement task unblocking
    (void)id; // Suppress unused parameter warning
    printf("unblock_task() called for PID %d. (Not yet implemented)\n", id);
}
