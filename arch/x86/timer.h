#ifndef _ARCH_X86_TIMER_H
#define _ARCH_X86_TIMER_H

#include <stdint.h>
#include "arch/x86/idt.h" // For registers_t, as timer_handler_c will use it

// --- PIT (Programmable Interval Timer 8253/8254) Registers/Ports ---
#define PIT_CHANNEL0_DATA_PORT 0x40 // Channel 0 data port (read/write)
#define PIT_CHANNEL1_DATA_PORT 0x41 // Channel 1 data port (not typically used by kernel timer)
#define PIT_CHANNEL2_DATA_PORT 0x42 // Channel 2 data port (used for PC speaker, etc.)
#define PIT_COMMAND_REGISTER   0x43 // Mode/Command register

// --- PIT Configuration Constants ---

// The PIT's internal oscillator frequency (approx 1.193182 MHz)
#define PIT_BASE_FREQUENCY 1193182
// More precise value: 3579545 / 3.0 = 1193181.666... Hz

// --- PIT Command Register Bits (for setting mode/command) ---

// Bits 7-6: Select Channel
//   00 = Channel 0
//   01 = Channel 1
//   10 = Channel 2
//   11 = Read-back command (8254 only)
#define PIT_CMD_SELECT_CHANNEL0   (0 << 6)
#define PIT_CMD_SELECT_CHANNEL1   (1 << 6)
#define PIT_CMD_SELECT_CHANNEL2   (2 << 6)
#define PIT_CMD_READ_BACK         (3 << 6) // For 8254 PIT

// Bits 5-4: Access Mode (how data is read/written to data ports)
//   00 = Latch count value command (for current count reading)
//   01 = LoByte only (lower 8 bits)
//   10 = HiByte only (upper 8 bits)
//   11 = LoByte then HiByte (16-bit value, LSB first then MSB)
#define PIT_CMD_ACCESS_LATCH      (0 << 4)
#define PIT_CMD_ACCESS_LOBYTE     (1 << 4)
#define PIT_CMD_ACCESS_HIBYTE     (2 << 4)
#define PIT_CMD_ACCESS_LOHIBYTE   (3 << 4) // Standard for setting 16-bit divisor

// Bits 3-1: Operating Mode
//   000 (0) = Mode 0: Interrupt on terminal count
//   001 (1) = Mode 1: Hardware re-triggerable one-shot
//   010 (2) = Mode 2: Rate Generator (periodic interrupt) - often used for system timer
//   011 (3) = Mode 3: Square Wave generator - also used for system timer
//   100 (4) = Mode 4: Software triggered strobe
//   101 (5) = Mode 5: Hardware triggered strobe
//   110 (x) = Mode 2 (undocumented alias)
//   111 (x) = Mode 3 (undocumented alias)
#define PIT_CMD_MODE0_INTERRUPT_ON_TERMINAL_COUNT (0 << 1)
#define PIT_CMD_MODE1_HW_RETRIGGERABLE_ONE_SHOT   (1 << 1)
#define PIT_CMD_MODE2_RATE_GENERATOR              (2 << 1)
#define PIT_CMD_MODE3_SQUARE_WAVE_GENERATOR       (3 << 1)
#define PIT_CMD_MODE4_SW_TRIGGERED_STROBE         (4 << 1)
#define PIT_CMD_MODE5_HW_TRIGGERED_STROBE         (5 << 1)

// Bit 0: BCD/Binary Mode
//   0 = 16-bit binary counter
//   1 = 4-digit BCD counter (rarely used in modern kernels)
#define PIT_CMD_BINARY_MODE       0x00 // Use 16-bit binary mode for divisor
#define PIT_CMD_BCD_MODE          0x01


// --- Function Declarations ---

/**
 * @brief Initializes the PIT Channel 0 to fire at a desired frequency.
 * This will configure PIT Channel 0 in a periodic mode (e.g., Mode 2 or 3)
 * and set its divisor to achieve the target frequency. IRQ0 will be generated.
 *
 * @param frequency The desired frequency in Hz for the timer interrupts.
 */
void pit_init(uint32_t frequency);

/**
 * @brief The C-level handler for IRQ0 (timer) interrupts.
 * This function is called from the assembly IRQ stub for ISR 32.
 * It typically increments a system tick counter and may perform task scheduling.
 *
 * @param regs Pointer to the register state pushed onto the stack by the ISR stub.
 */
void timer_handler_c(registers_t* regs);

/**
 * @brief (Optional) Retrieves the current system tick count.
 * The tick count is incremented by `timer_handler_c` on each timer interrupt.
 *
 * @return uint64_t The total number of timer ticks since PIT initialization.
 *                  (Note: Needs a static variable in timer.c to store ticks)
 */
uint64_t get_system_ticks(void);


#endif // _ARCH_X86_TIMER_H
