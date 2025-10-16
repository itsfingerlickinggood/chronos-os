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

// --- Cursor Control ---

/**
 * @brief Sets the cursor position to a specific row and column.
 * @param row The row to position the cursor (0 to VGA_HEIGHT-1).
 * @param col The column to position the cursor (0 to VGA_WIDTH-1).
 */
void vga_set_cursor_pos(int row, int col);

/**
 * @brief Gets the current cursor position.
 * @param row Pointer to store the current row (can be NULL).
 * @param col Pointer to store the current column (can be NULL).
 */
void vga_get_cursor_pos(int* row, int* col);

// --- Coordinate-based Character/String Writes ---

/**
 * @brief Writes a character at a specific position with the current style.
 * Does not move the cursor or trigger scrolling.
 * @param c The character to write.
 * @param row The row to write at (0 to VGA_HEIGHT-1).
 * @param col The column to write at (0 to VGA_WIDTH-1).
 */
void vga_putc_at(char c, int row, int col);

/**
 * @brief Writes a character at a specific position with a specific style.
 * Does not move the cursor or trigger scrolling.
 * @param c The character to write.
 * @param style The style byte to use.
 * @param row The row to write at (0 to VGA_HEIGHT-1).
 * @param col The column to write at (0 to VGA_WIDTH-1).
 */
void vga_putc_at_styled(char c, uint8_t style, int row, int col);

/**
 * @brief Writes a string at a specific position with the current style.
 * Does not move the cursor or trigger scrolling.
 * Will stop at screen boundaries (no wrapping).
 * @param str The string to write.
 * @param row The row to start writing at (0 to VGA_HEIGHT-1).
 * @param col The column to start writing at (0 to VGA_WIDTH-1).
 */
void vga_puts_at(const char* str, int row, int col);

/**
 * @brief Writes a string at a specific position with a specific style.
 * Does not move the cursor or trigger scrolling.
 * Will stop at screen boundaries (no wrapping).
 * @param str The string to write.
 * @param style The style byte to use.
 * @param row The row to start writing at (0 to VGA_HEIGHT-1).
 * @param col The column to start writing at (0 to VGA_WIDTH-1).
 */
void vga_puts_at_styled(const char* str, uint8_t style, int row, int col);

// --- Region Fill Helpers ---

/**
 * @brief Fills a rectangular region with a specific character and style.
 * @param row The starting row (0 to VGA_HEIGHT-1).
 * @param col The starting column (0 to VGA_WIDTH-1).
 * @param width The width of the rectangle.
 * @param height The height of the rectangle.
 * @param c The character to fill with.
 * @param style The style byte to use.
 */
void vga_fill_rect(int row, int col, int width, int height, char c, uint8_t style);

/**
 * @brief Clears a rectangular region (fills with spaces) using the current style.
 * @param row The starting row (0 to VGA_HEIGHT-1).
 * @param col The starting column (0 to VGA_WIDTH-1).
 * @param width The width of the rectangle.
 * @param height The height of the rectangle.
 */
void vga_clear_rect(int row, int col, int width, int height);

/**
 * @brief Draws a horizontal line at a specific position.
 * @param row The row to draw at.
 * @param col The starting column.
 * @param length The length of the line.
 * @param c The character to use for the line.
 * @param style The style byte to use.
 */
void vga_draw_hline(int row, int col, int length, char c, uint8_t style);

/**
 * @brief Draws a vertical line at a specific position.
 * @param row The starting row.
 * @param col The column to draw at.
 * @param length The length of the line.
 * @param c The character to use for the line.
 * @param style The style byte to use.
 */
void vga_draw_vline(int row, int col, int length, char c, uint8_t style);

/**
 * @brief Draws a box (border) at a specific position.
 * @param row The starting row.
 * @param col The starting column.
 * @param width The width of the box (including borders).
 * @param height The height of the box (including borders).
 * @param style The style byte to use.
 */
void vga_draw_box(int row, int col, int width, int height, uint8_t style);

// --- Constants for Box Drawing (ASCII-safe for monochrome compatibility) ---
#define VGA_BOX_HORIZONTAL '-'
#define VGA_BOX_VERTICAL '|'
#define VGA_BOX_TOP_LEFT '+'
#define VGA_BOX_TOP_RIGHT '+'
#define VGA_BOX_BOTTOM_LEFT '+'
#define VGA_BOX_BOTTOM_RIGHT '+'
#define VGA_BOX_CROSS '+'

#endif // _KERNEL_VGA_H
