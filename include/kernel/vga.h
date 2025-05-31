#ifndef _KERNEL_VGA_H
#define _KERNEL_VGA_H

#include <stdint.h> // For uint8_t, uint16_t

// --- VGA Constants ---
#define VGA_MEMORY_ADDRESS 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

// --- VGA Color Definitions ---
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14, // Often perceived as Yellow
    VGA_COLOR_WHITE = 15,
};

// --- VGA Entry Styling Helper ---

/**
 * @brief Creates a VGA style byte from foreground and background colors.
 * @param fg Foreground color (enum vga_color).
 * @param bg Background color (enum vga_color).
 * @return uint8_t The combined style byte.
 */
static inline uint8_t vga_entry_style(enum vga_color fg, enum vga_color bg) {
    return (uint8_t)fg | ((uint8_t)bg << 4);
}

// --- VGA Character Entry Helper ---
// A VGA character entry is a 16-bit value:
// Bits 0-7: ASCII character code
// Bits 8-11: Foreground color
// Bits 12-15: Background color

/**
 * @brief Creates a 16-bit VGA character entry from a character and a style byte.
 * @param uc The character to display.
 * @param style The style byte (foreground and background color).
 * @return uint16_t The 16-bit value to be written to VGA memory.
 */
static inline uint16_t vga_entry(unsigned char uc, uint8_t style) {
    return (uint16_t)uc | ((uint16_t)style << 8);
}

// --- VGA Driver Function Declarations ---

/**
 * @brief Initializes the VGA driver.
 * (e.g., clears screen, sets default style, positions cursor).
 */
void vga_init(void);

/**
 * @brief Puts a single character to the screen at the current cursor position.
 * Handles newlines, tabs (basic), and scrolling.
 * @param c The character to print.
 */
void vga_putc(char c);

/**
 * @brief Puts a null-terminated string to the screen.
 * @param str The string to print.
 */
void vga_puts(const char* str);

/**
 * @brief Sets the current drawing style (foreground and background color).
 * @param style The style byte to use for subsequent characters.
 */
void vga_set_style(uint8_t style);

/**
 * @brief Gets the current drawing style.
 * @return uint8_t The current style byte.
 */
uint8_t vga_get_style(void);

/**
 * @brief Clears the entire screen with the current background color (or a specific one).
 */
void vga_clear_screen(void);

// Future function declarations (cursor, scrolling control, etc.) could go here.
// void vga_set_cursor_pos(int row, int col);
// void vga_get_cursor_pos(int* row, int* col);

#endif // _KERNEL_VGA_H
