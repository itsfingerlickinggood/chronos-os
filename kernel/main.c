#include "kernel/vga.h"       // For vga_init
#include "kernel/printf.h"    // For kprintf
#include "kernel/scheduler.h" // Includes pcb_t, task_state, create_task, schedule, current_task, tasks, MAX_TASKS
#include "kernel/memory.h"    // Includes kmalloc, kfree, memory_init
#include "kernel/ai_core.h"   // Not used in this part, but kept
#include "arch/x86/idt.h"     // For idt_init
#include "arch/x86/pic.h"     // For pic_remap, irq_clear_mask
#include "arch/x86/timer.h"   // For pit_init, get_system_ticks, timer_handler_c

// --- Dummy Task Functions (Revised for Preemptive Test) ---
void task_A_func(void) {
    uint32_t counter = 0;
    // Note: current_task might not be set when this first line executes due to context switch timing.
    // A proper way for a task to get its PID would be a syscall or argument.
    // kprintf("Task A (PID %d) started.\n", current_task ? current_task->id : 0);
    while (1) {
        kprintf("A:%d ", counter++);
        for (volatile int i = 0; i < 500000; ++i); // Simulate work
        if (counter > 50 && counter % 20 == 0) kprintf("\n"); // Newline occasionally for readability
        // Tasks now run indefinitely for preemptive test
    }
}

void task_B_func(void) {
    uint32_t counter = 0;
    // kprintf("Task B (PID %d) started.\n", current_task ? current_task->id : 0);
    while (1) {
        kprintf("B:%d ", counter++);
        for (volatile int i = 0; i < 500000; ++i);
        if (counter > 50 && counter % 20 == 0) kprintf("\n");
    }
}

void task_C_func(void) {
    uint32_t counter = 0;
    // kprintf("Task C (PID %d) started.\n", current_task ? current_task->id : 0);
    while (1) {
        kprintf("C:%d ", counter++);
        for (volatile int i = 0; i < 500000; ++i);
        if (counter > 50 && counter % 20 == 0) kprintf("\n");
    }
}


// --- Kernel Main ---
int kmain() {
    vga_init(); // Initialize VGA display first
    kprintf("Kernel main: VGA initialized.\n");

    // Initialize Interrupt Descriptor Table (IDT) and Programmable Interrupt Controller (PIC)
    idt_init();
    pic_remap(0x20, 0x28); // Remap PIC IRQs: Master 0x20-0x27 (32-39), Slave 0x28-0x2F (40-47)
    kprintf("Kernel main: IDT initialized, PIC remapped.\n");

    // Initialize PIT (Programmable Interval Timer)
    uint32_t timer_frequency = 100; // Hz
    pit_init(timer_frequency);
    kprintf("PIT initialized to %d Hz.\n", timer_frequency);

    // Enable (unmask) IRQ0 (the timer line) on the PIC
    irq_clear_mask(0);
    kprintf("IRQ0 (timer) unmasked on PIC.\n");

    // --- Optional: Test a CPU exception (e.g., Divide by Zero) ---
    // This will call ISR 0 and should be handled by fault_handler, then halt.
    // Make sure this is commented out for normal preemptive scheduler testing.
    /*
    kprintf("Testing divide-by-zero exception...\n");
    volatile int x = 5;
    volatile int y = 0;
    volatile int z = x / y;
    kprintf("Value of z (should not be reached): %d\n", z);
    */
    // --- End of Exception Test ---


    memory_init(); // Initialize the memory manager
    kprintf("Kernel main: Memory manager started.\n");

    task_init_system(); // Initialize the scheduler system
    kprintf("Kernel main: Scheduler system initialized.\n");

    // Run memory tests (optional, can be commented out for scheduler focus)
    // run_memory_tests(); // This would need kprintf internally too
    // kprintf("Kernel main: Memory tests complete.\n");

    kprintf("\nCreating tasks for preemptive test...\n");
    pid_t pidA = create_task(task_A_func, 0); // priority 0
    kprintf("Created Task A with PID %d\n", pidA);

    pid_t pidB = create_task(task_B_func, 0); // priority 0
    kprintf("Created Task B with PID %d\n", pidB);

    pid_t pidC = create_task(task_C_func, 0); // Optional third task
    kprintf("Created Task C with PID %d\n", pidC);

    kprintf("\nEnabling interrupts and starting scheduler (via timer ticks). Kernel idling.\n");
    asm volatile ("sti"); // Enable interrupts globally. Timer will start firing.

    // Kernel idle loop. Tasks will run via preemption from the timer interrupt.
    // The first task switch will happen from this kmain context via the timer ISR calling schedule(),
    // which will then call context_switch_asm. The dummy_kernel_esp_storage in schedule()
    // will be used to "save" this kmain context (which won't be restored).
    while (1) {
        // If all tasks were to terminate and current_task becomes NULL (scheduler idles),
        // this hlt would be continuously interrupted by the timer, which would call schedule,
        // find no tasks, and return here. This is a safe idle state.
        asm volatile ("hlt"); // Wait for the next interrupt (e.g., timer tick)
    }

    // This part will not be reached.
    return 0;
}
