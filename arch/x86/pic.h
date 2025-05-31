#ifndef _ARCH_X86_PIC_H
#define _ARCH_X86_PIC_H

#include <stdint.h>

// Default base offsets for IRQs in the IDT (before remapping)
// These are the values the PICs are typically initialized to by the BIOS.
// #define PIC1_DEFAULT_OFFSET 0x08 // Master PIC default offset
// #define PIC2_DEFAULT_OFFSET 0x70 // Slave PIC default offset (often, but can vary)
// After remapping, we'll typically use 0x20 for Master (IRQ 0-7 -> IDT 32-39)
// and 0x28 for Slave (IRQ 8-15 -> IDT 40-47).

// I/O Port addresses for PICs
#define PIC1_COMMAND_PORT 0x20  // Master PIC command/status port
#define PIC1_DATA_PORT    0x21  // Master PIC data port
#define PIC2_COMMAND_PORT 0xA0  // Slave PIC command/status port
#define PIC2_DATA_PORT    0xA1  // Slave PIC data port

// Initialization Command Words (ICWs)
// ICW1: Initialization command
#define ICW1_INIT           0x10    // Required to start initialization sequence
#define ICW1_ICW4_EXPECT    0x01    // If set, PIC expects to receive ICW4
#define ICW1_SINGLE         0x02    // If set, PIC operates in single mode (no slave)
#define ICW1_INTERVAL4      0x04    // If set, call address interval is 4, else 8
#define ICW1_LEVEL_TRIGGERED 0x08   // If set, level triggered mode, else edge triggered

// ICW4: Operating modes
#define ICW4_8086_MODE      0x01    // 8086/88 (MCS-80/85) mode
#define ICW4_AUTO_EOI       0x02    // Auto EOI mode
#define ICW4_BUFFER_SLAVE   0x08    // Buffered mode (slave)
#define ICW4_BUFFER_MASTER  0x0C    // Buffered mode (master)
#define ICW4_SFNM           0x10    // Special Fully Nested Mode

// PIC Command Word 2 (OCW2)
#define PIC_EOI             0x20    // End-Of-Interrupt command code

// PIC Command Word 3 (OCW3)
#define PIC_READ_IRR        0x0A    // Read Interrupt Request Register command
#define PIC_READ_ISR        0x0B    // Read In-Service Register command


// --- Function Declarations ---

/**
 * @brief Remaps the PIC IRQs to new base offsets in the IDT.
 * Standard practice is to remap IRQs 0-7 to IDT entries 32-39 (offset1=0x20)
 * and IRQs 8-15 to IDT entries 40-47 (offset2=0x28).
 * @param offset1 New base offset for the Master PIC (IRQs 0-7).
 * @param offset2 New base offset for the Slave PIC (IRQs 8-15).
 */
void pic_remap(int offset1, int offset2);

/**
 * @brief Sends an End-Of-Interrupt (EOI) signal to the PIC(s).
 * If the IRQ came from the slave PIC, an EOI must be sent to both.
 * @param irq The IRQ number (0-15) that has been handled.
 */
void pic_send_eoi(unsigned char irq);

/**
 * @brief Masks (disables) a specific IRQ line.
 * @param irq_line The IRQ line number (0-15) to mask.
 */
void irq_set_mask(unsigned char irq_line);

/**
 * @brief Clears the mask for a specific IRQ line (enables it).
 * @param irq_line The IRQ line number (0-15) to unmask.
 */
void irq_clear_mask(unsigned char irq_line);

/**
 * @brief Reads the Interrupt Request Register (IRR) for both PICs.
 * The IRR indicates which IRQs are currently requesting service (pending).
 * @return uint16_t A 16-bit value where each bit corresponds to an IRQ line (0-15).
 *                  Bit 0 for IRQ0, ..., Bit 15 for IRQ15.
 */
uint16_t pic_get_irr(void);

/**
 * @brief Reads the In-Service Register (ISR) for both PICs.
 * The ISR indicates which IRQs are currently being serviced.
 * @return uint16_t A 16-bit value similar to IRR, but for in-service IRQs.
 */
uint16_t pic_get_isr(void);

#endif // _ARCH_X86_PIC_H
