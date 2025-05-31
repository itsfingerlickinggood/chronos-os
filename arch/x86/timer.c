#include "arch/x86/timer.h"
#include "arch/x86/io.h"    // For outb
#include "kernel/printf.h"  // For kprintf (optional, for debugging)
#include "kernel/scheduler.h" // For schedule()

// Volatile global variable to store the system tick count.
// This will be incremented by the timer_handler_c.
// 'volatile' is important as it's modified by an interrupt handler.
static volatile uint64_t system_ticks = 0;


/**
 * @brief Initializes the PIT (Programmable Interval Timer) Channel 0.
 *
 * Configures PIT Channel 0 to generate interrupts at a specified frequency.
 * This involves calculating a divisor for the PIT's base frequency, then
 * sending a command byte and the divisor (LSB then MSB) to the PIT's ports.
 *
 * @param frequency The desired interrupt frequency in Hz. If 0, a default
 *                  (e.g., 100Hz) is used. If too high, it's capped.
 */
void pit_init(uint32_t frequency) {
    uint32_t actual_frequency;

    if (frequency == 0) {
        kprintf("PIT Warning: Frequency cannot be 0. Using default 100Hz.\n");
        frequency = 100; // Default to a safe frequency
    }

    // The PIT divisor is a 16-bit value.
    // A divisor of 0 is interpreted as 65536 (2^16).
    // A divisor of 1 gives PIT_BASE_FREQUENCY.
    uint16_t divisor;
    if (frequency >= PIT_BASE_FREQUENCY) {
        // Cap at max frequency, prevent divisor < 1
        divisor = 1;
        actual_frequency = PIT_BASE_FREQUENCY;
        if (frequency > PIT_BASE_FREQUENCY) {
             kprintf("PIT Warning: Frequency %dHz is too high. Max is %dHz. Using max.\n", frequency, PIT_BASE_FREQUENCY);
        }
    } else {
        // Calculate the divisor.
        // Divisor = Base Frequency / Target Frequency
        uint32_t calculated_divisor = PIT_BASE_FREQUENCY / frequency;

        // Basic rounding: if the remainder is more than half the frequency, round up divisor.
        // This attempts to get closer to the target frequency.
        if (PIT_BASE_FREQUENCY % frequency > frequency / 2) {
            calculated_divisor++;
        }

        if (calculated_divisor > 0xFFFF) { // Max 16-bit value
            divisor = 0; // Will be treated as 65536 by PIT, giving lowest frequency
            actual_frequency = PIT_BASE_FREQUENCY / 65536;
            kprintf("PIT Warning: Frequency %dHz is too low. Min is ~18.2Hz. Using min.\n", frequency);
        } else if (calculated_divisor == 0) { // Should not happen if freq < base_freq
             divisor = 1; // Should be caught by freq >= PIT_BASE_FREQUENCY
             actual_frequency = PIT_BASE_FREQUENCY;
        }
        else {
            divisor = (uint16_t)calculated_divisor;
            actual_frequency = PIT_BASE_FREQUENCY / divisor;
        }
    }

    kprintf("PIT: Requested Freq: %d Hz, Divisor: %d (0x%x), Actual Freq: ~%d Hz\n",
            frequency, divisor, divisor, actual_frequency);

    // Send command byte to PIT Command Register:
    //   Channel 0 selected (bits 7-6 = 00)
    //   Access mode: LoByte/HiByte (bits 5-4 = 11) - LSB first, then MSB
    //   Operating mode: Mode 2 - Rate Generator (bits 3-1 = 010)
    //   BCD/Binary mode: 16-bit binary (bit 0 = 0)
    uint8_t command = PIT_CMD_SELECT_CHANNEL0 | \
                      PIT_CMD_ACCESS_LOHIBYTE | \
                      PIT_CMD_MODE2_RATE_GENERATOR | \
                      PIT_CMD_BINARY_MODE;

    outb(PIT_COMMAND_REGISTER, command);
    io_wait(); // Short delay often good practice after command

    // Send the divisor value (LSB first, then MSB) to Channel 0 data port
    uint8_t lsb = (uint8_t)(divisor & 0xFF);
    uint8_t msb = (uint8_t)((divisor >> 8) & 0xFF);

    outb(PIT_CHANNEL0_DATA_PORT, lsb);
    io_wait(); // Delay between LSB and MSB writes might be needed by some hardware
    outb(PIT_CHANNEL0_DATA_PORT, msb);
    io_wait();

    kprintf("PIT Channel 0 configured for ~%d Hz.\n", actual_frequency);
}

/**
 * @brief The C-level handler for IRQ0 (timer) interrupts.
 *
 * This function is called from the assembly IRQ stub for ISR 32 (IRQ 0).
 * It increments the global system tick counter.
 * In a multitasking kernel, this is also where task preemption/scheduling
 * would typically be triggered.
 *
 * @param regs Pointer to the register state pushed onto the stack by the ISR stub.
 *             This parameter is unused in this basic handler but is part of the
 *             standard signature for interrupt handlers.
 */
void timer_handler_c(registers_t* regs) {
    (void)regs; // Suppress unused parameter warning for now

    system_ticks++;

    // For debugging, print a message every N ticks:
    // Note: kprintf doesn't support %llu for uint64_t directly.
    // Casting to uint32_t for printing if ticks aren't expected to overflow uint32_t quickly.
    // if (system_ticks % 100 == 0) { // Example: every 100 ticks (1 second if 100Hz)
    //    kprintf("Timer tick: %d (IRQ0), calling scheduler.\n", (uint32_t)system_ticks);
    // }

    // Call the scheduler to potentially switch tasks (preemption)
    schedule();
}

/**
 * @brief Retrieves the current system tick count.
 *
 * The tick count is incremented by `timer_handler_c` on each timer interrupt.
 *
 * @return uint64_t The total number of timer ticks since PIT initialization.
 */
uint64_t get_system_ticks(void) {
    // TODO: Consider atomicity if reading from a different context than the interrupt handler
    // on a multi-core system or if interrupts can interrupt this read on a single core.
    // For a simple single-core kernel without preemption during this read, direct access is okay.
    // A lock (cli/sti) or atomic read operation might be needed in more complex scenarios.
    return system_ticks;
}
