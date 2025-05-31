#ifndef KERNEL_SCHEDULER_H
#define KERNEL_SCHEDULER_H

#include <stdint.h> // For uintptr_t

// Process ID type
typedef int pid_t;

// Task states
enum task_state {
    TASK_UNUSED,    // PCB is not used
    TASK_READY,     // Task is ready to run
    TASK_RUNNING,   // Task is currently running
    TASK_WAITING,   // Task is waiting for an event (e.g., I/O, semaphore)
    TASK_SLEEPING,  // Task is sleeping for a duration
    TASK_TERMINATED // Task has finished execution
};

// Kernel stack size for tasks
#define KERNEL_STACK_SIZE (4096) // 4KB stack

// Process Control Block (PCB)
typedef struct pcb_t {
    pid_t id;                       // Process ID
    enum task_state state;          // Current state of the task

    // Context switching information
    uintptr_t stack_pointer;        // Saved stack pointer (e.g., ESP)
    uintptr_t instruction_pointer;  // Saved instruction pointer (e.g., EIP)

    void* stack_base;               // Base of the allocated stack (for kfree when task terminates)
    // void (*entry_point)(void);   // Initial entry point, could be stored here or IP set directly
    int priority;                   // Task priority

    struct pcb_t* next;             // Pointer for linking in ready queue, wait queue, etc.

    // Future extensions:
    // - General purpose registers
    // - Page directory / memory mapping info
    // - Open file descriptors
    // - Priority level
    // - Signals / IPC data
    // - Accounting information (CPU time used, etc.)
} pcb_t;

// Placeholder for scheduler function declarations
// (This will be expanded later)
void schedule(void); // The main scheduler function to pick the next task
void task_init_system(void); // Initialize the scheduler system (e.g., PCB array)
pid_t create_task(void (*entry_point)(void), int priority); // Create a new task (returns PID or error)
void terminate_task(pid_t id); // Terminate a task
void block_task(pid_t id, enum task_state reason); // Block the current task
void unblock_task(pid_t id); // Unblock a task (move to ready)

// External declarations for global scheduler variables (for inspection/debug)
// NOTE: Direct access is generally discouraged; prefer accessor functions or syscalls.
extern pcb_t* current_task;
extern pcb_t tasks[]; // Exposes the whole array; use with caution. MAX_TASKS needs to be known or passed.


#endif // KERNEL_SCHEDULER_H
