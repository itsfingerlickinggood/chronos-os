#ifndef _ARCH_X86_IO_H
#define _ARCH_X86_IO_H

#include <stdint.h>

/**
 * @brief Outputs a byte to the specified I/O port.
 * @param port The I/O port number.
 * @param val The byte value to output.
 */
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

/**
 * @brief Inputs a byte from the specified I/O port.
 * @param port The I/O port number.
 * @return uint8_t The byte value read from the port.
 */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

/**
 * @brief Outputs a word (16-bit) to the specified I/O port.
 * @param port The I/O port number.
 * @param val The word value to output.
 */
static inline void outw(uint16_t port, uint16_t val) {
    asm volatile ("outw %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

/**
 * @brief Inputs a word (16-bit) from the specified I/O port.
 * @param port The I/O port number.
 * @return uint16_t The word value read from the port.
 */
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

/**
 * @brief Outputs a long (32-bit) to the specified I/O port.
 * @param port The I/O port number.
 * @param val The long value to output.
 */
static inline void outl(uint16_t port, uint32_t val) {
    asm volatile ("outl %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

/**
 * @brief Inputs a long (32-bit) from the specified I/O port.
 * @param port The I/O port number.
 * @return uint32_t The long value read from the port.
 */
static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

/**
 * @brief Provides a short delay, often used for I/O operations with older hardware.
 * This typically involves writing to an unused port (like 0x80, often used for POST codes).
 */
static inline void io_wait(void) {
    // Using outb to port 0x80 is a common way to introduce a small I/O delay.
    // This port is often used for POST codes by the BIOS and is generally safe to write to.
    outb(0x80, 0);
}

#endif // _ARCH_X86_IO_H
