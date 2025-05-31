#include "arch/x86/pic.h"
#include "arch/x86/io.h" // For outb, inb, io_wait

/**
 * @brief Remaps the Programmable Interrupt Controllers (PICs).
 *
 * The 8259A PICs are typically initialized by the BIOS to deliver IRQs
 * at IDT vectors 0x08-0x0F (master) and 0x70-0x77 (slave). These conflict
 * with CPU exceptions (0x00-0x1F). This function remaps them to safer offsets.
 * A common configuration is:
 * - Master PIC (IRQs 0-7) maps to IDT vectors offset1 to offset1+7.
 * - Slave PIC (IRQs 8-15) maps to IDT vectors offset2 to offset2+7.
 *
 * @param offset1 The new base IDT vector for the master PIC (e.g., 0x20 for IDT 32).
 * @param offset2 The new base IDT vector for the slave PIC (e.g., 0x28 for IDT 40).
 */
void pic_remap(int offset1, int offset2) {
    unsigned char master_mask, slave_mask;

    // 1. Save current masks from data ports
    master_mask = inb(PIC1_DATA_PORT);
    io_wait();
    slave_mask = inb(PIC2_DATA_PORT);
    io_wait();

    // 2. Start initialization sequence (ICW1)
    // ICW1_INIT: Starts initialization.
    // ICW1_ICW4_EXPECT: Tells PIC to expect ICW4.
    outb(PIC1_COMMAND_PORT, ICW1_INIT | ICW1_ICW4_EXPECT);
    io_wait();
    outb(PIC2_COMMAND_PORT, ICW1_INIT | ICW1_ICW4_EXPECT);
    io_wait();

    // 3. Send ICW2: Vector offsets for master and slave PICs
    // This maps IRQ_0 to IDT_vector(offset1), IRQ_1 to IDT_vector(offset1+1), etc.
    outb(PIC1_DATA_PORT, (unsigned char)offset1); // Master PIC new base offset
    io_wait();
    outb(PIC2_DATA_PORT, (unsigned char)offset2); // Slave PIC new base offset
    io_wait();

    // 4. Send ICW3: Configure master-slave relationship
    // Tell Master PIC that there is a slave PIC at IRQ2 (binary 00000100)
    outb(PIC1_DATA_PORT, 0x04);
    io_wait();
    // Tell Slave PIC its cascade identity (binary 00000010, corresponding to IRQ2 on master)
    outb(PIC2_DATA_PORT, 0x02);
    io_wait();

    // 5. Send ICW4: Set operating mode (e.g., 8086/88 mode)
    outb(PIC1_DATA_PORT, ICW4_8086_MODE);
    io_wait();
    outb(PIC2_DATA_PORT, ICW4_8086_MODE);
    io_wait();

    // 6. Restore saved masks
    outb(PIC1_DATA_PORT, master_mask);
    io_wait();
    outb(PIC2_DATA_PORT, slave_mask);
    io_wait();
}

/**
 * @brief Sends an End-Of-Interrupt (EOI) signal to the PIC(s).
 *
 * If the IRQ originated from the slave PIC (IRQ 8-15), an EOI must be
 * sent to both the slave and the master PICs. Otherwise, only to the master.
 * @param irq The IRQ number (0-15) that has been handled.
 */
void pic_send_eoi(unsigned char irq) {
    if (irq >= 8 && irq <= 15) { // IRQ came from the Slave PIC
        outb(PIC2_COMMAND_PORT, PIC_EOI); // Send EOI to Slave PIC
    }
    // Always send EOI to Master PIC
    outb(PIC1_COMMAND_PORT, PIC_EOI);
}

/**
 * @brief Masks (disables) a specific IRQ line.
 * @param irq_line The IRQ line number (0-15) to mask.
 */
void irq_set_mask(unsigned char irq_line) {
    uint16_t port;
    uint8_t value;

    if (irq_line < 8) { // IRQ 0-7 are on Master PIC
        port = PIC1_DATA_PORT;
    } else { // IRQ 8-15 are on Slave PIC
        port = PIC2_DATA_PORT;
        irq_line -= 8; // Adjust to 0-7 for slave's bitmask
    }
    // Read current mask, set the bit for the IRQ line, then write back
    value = inb(port) | (1 << irq_line);
    outb(port, value);
}

/**
 * @brief Clears the mask for a specific IRQ line (enables it).
 * @param irq_line The IRQ line number (0-15) to unmask.
 */
void irq_clear_mask(unsigned char irq_line) {
    uint16_t port;
    uint8_t value;

    if (irq_line < 8) { // IRQ 0-7 are on Master PIC
        port = PIC1_DATA_PORT;
    } else { // IRQ 8-15 are on Slave PIC
        port = PIC2_DATA_PORT;
        irq_line -= 8; // Adjust to 0-7 for slave's bitmask
    }
    // Read current mask, clear the bit for the IRQ line, then write back
    value = inb(port) & ~(1 << irq_line);
    outb(port, value);
}

/**
 * @brief Helper function to read a register from both PICs.
 * Used for IRR and ISR.
 * @param ocw3 The OCW3 command to select IRR or ISR.
 * @return uint16_t Combined 16-bit value (Slave in MSB, Master in LSB).
 */
static uint16_t pic_read_register(uint8_t ocw3_command) {
    // Issue OCW3 to select the register to read (IRR or ISR)
    outb(PIC1_COMMAND_PORT, ocw3_command);
    outb(PIC2_COMMAND_PORT, ocw3_command);
    // Read from data ports. Slave PIC's value is in the high byte.
    return ((uint16_t)inb(PIC2_DATA_PORT) << 8) | inb(PIC1_DATA_PORT);
}

/**
 * @brief Reads the combined Interrupt Request Register (IRR) from both PICs.
 * IRR shows which interrupts are pending (requested but not yet acknowledged).
 * @return uint16_t 16-bit mask of pending IRQs.
 */
uint16_t pic_get_irr(void) {
    return pic_read_register(PIC_READ_IRR);
}

/**
 * @brief Reads the combined In-Service Register (ISR) from both PICs.
 * ISR shows which interrupts are currently being serviced (acknowledged and handler running).
 * @return uint16_t 16-bit mask of in-service IRQs.
 */
uint16_t pic_get_isr(void) {
    return pic_read_register(PIC_READ_ISR);
}
