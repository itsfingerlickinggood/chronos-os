#include "kernel/vga.h"
#include <stddef.h> // For NULL (used in vga_puts)

// --- Static Global Variables ---
static int term_row;
static int term_col;
static uint8_t term_style;
static uint16_t* term_buffer; // Pointer to VGA buffer (0xB8000)

// --- Helper Functions ---

/**
 * @brief Puts a character with a given style at a specific screen location.
 * Does not update global cursor position or handle scrolling.
 * @param c Character to put.
 * @param style Style byte for the character.
 * @param row Row to place the character.
 * @param col Column to place the character.
 */
static void vga_put_char_at(char c, uint8_t style, int row, int col) {
    if ((unsigned int)row >= VGA_HEIGHT || (unsigned int)col >= VGA_WIDTH) {
        return; // Out of bounds
    }
    term_buffer[row * VGA_WIDTH + col] = vga_entry((unsigned char)c, style);
}

/**
 * @brief Scrolls the terminal content up by one line.
 * The top line is lost, and the bottom line is cleared.
 * Cursor is moved to the beginning of the new last line.
 */
static void vga_scroll(void) {
    // Move all lines up by one
    for (int y = 0; y < VGA_HEIGHT - 1; ++y) {
        for (int x = 0; x < VGA_WIDTH; ++x) {
            term_buffer[y * VGA_WIDTH + x] = term_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    // Clear the last line
    uint16_t blank_entry = vga_entry(' ', term_style); // Use current style for blank
    for (int x = 0; x < VGA_WIDTH; ++x) {
        term_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = blank_entry;
    }
    term_row = VGA_HEIGHT - 1; // Cursor stays on the last line after scroll
    term_col = 0;              // Reset column to the beginning of the line
}


// --- Public VGA Driver Functions ---

/**
 * @brief Initializes the VGA terminal.
 * Sets up the terminal buffer, default style, clears the screen, and positions cursor.
 */
void vga_init(void) {
    term_row = 0;
    term_col = 0;
    term_style = vga_entry_style(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    term_buffer = (uint16_t*) VGA_MEMORY_ADDRESS; // Pointer to VGA video memory
    vga_clear_screen();
    // Hardware cursor update would go here if implemented, e.g., update_cursor(term_row, term_col);
}

/**
 * @brief Clears the entire screen with the current background color.
 * Resets cursor to top-left (0,0).
 */
void vga_clear_screen(void) {
    // Create a blank entry with the current style (respects background color)
    uint16_t blank_entry = vga_entry(' ', term_style);
    for (int y = 0; y < VGA_HEIGHT; ++y) {
        for (int x = 0; x < VGA_WIDTH; ++x) {
            term_buffer[y * VGA_WIDTH + x] = blank_entry;
        }
    }
    term_row = 0;
    term_col = 0;
    // Hardware cursor update would go here: update_cursor(term_row, term_col);
}

/**
 * @brief Sets the current drawing style (foreground and background color).
 * @param style The style byte to use for subsequent characters.
 */
void vga_set_style(uint8_t style) {
    term_style = style;
}

/**
 * @brief Gets the current drawing style.
 * @return uint8_t The current style byte.
 */
uint8_t vga_get_style(void) {
    return term_style;
}

/**
 * @brief Puts a single character to the screen at the current cursor position.
 * Handles newlines, tabs (basic expansion), backspace, and scrolling.
 * @param c The character to print.
 */
void vga_putc(char c) {
    unsigned char uc = (unsigned char)c;

    if (uc == '\n') {
        term_col = 0;
        term_row++;
    } else if (uc == '\r') {
        term_col = 0;
    } else if (uc == '\t') {
        // Expand tab to align to the next multiple of a tab stop (e.g., 4 or 8)
        const int tab_stop_width = 4;
        int spaces_to_tab = tab_stop_width - (term_col % tab_stop_width);
        for (int i = 0; i < spaces_to_tab; ++i) {
            // Recursive call to handle potential line wrapping or scrolling for each space
            // This check prevents infinite recursion if VGA_WIDTH is not multiple of tab_stop_width
            if (term_col < VGA_WIDTH -1) {
                 vga_putc(' ');
            } else { // at the edge, just wrap
                vga_putc(' '); // this will wrap the line
            }
        }
        return; // vga_putc for space will handle cursor advancement & scroll
    } else if (uc == '\b') { // Simple backspace
        if (term_col > 0) {
            term_col--;
            // Erase character at new cursor position by writing a space with current style
            vga_put_char_at(' ', term_style, term_row, term_col);
        } else if (term_row > 0) { // Backspace at start of line, wrap to previous line
            term_row--;
            term_col = VGA_WIDTH - 1;
            // Optionally, don't erase, just move cursor. For now, erase.
            vga_put_char_at(' ', term_style, term_row, term_col);
        }
        // Note: This backspace doesn't correctly handle multi-line "upwards" deletion.
    } else { // Printable character
        vga_put_char_at(uc, term_style, term_row, term_col);
        term_col++;
    }

    // If cursor went off screen horizontally (line wrap)
    if (term_col >= VGA_WIDTH) {
        term_col = 0;
        term_row++;
    }

    // If cursor went off screen vertically (or due to newline/col wrap that pushed it down)
    if (term_row >= VGA_HEIGHT) {
        vga_scroll();
        // term_row is reset to VGA_HEIGHT - 1 by vga_scroll()
        // term_col is reset to 0 by vga_scroll()
    }
    // Hardware cursor update would go here: update_cursor(term_row, term_col);
}

/**
 * @brief Puts a null-terminated string to the screen.
 * Uses vga_putc for each character to handle special characters and scrolling.
 * @param str The string to print.
 */
void vga_puts(const char* str) {
    if (!str) {
        return;
    }
    for (size_t i = 0; str[i] != '\0'; ++i) {
        vga_putc(str[i]);
    }
}

// --- Cursor Control Functions ---

/**
 * @brief Sets the cursor position to a specific row and column.
 * @param row The row to position the cursor (0 to VGA_HEIGHT-1).
 * @param col The column to position the cursor (0 to VGA_WIDTH-1).
 */
void vga_set_cursor_pos(int row, int col) {
    if (row >= 0 && row < VGA_HEIGHT && col >= 0 && col < VGA_WIDTH) {
        term_row = row;
        term_col = col;
    }
}

/**
 * @brief Gets the current cursor position.
 * @param row Pointer to store the current row (can be NULL).
 * @param col Pointer to store the current column (can be NULL).
 */
void vga_get_cursor_pos(int* row, int* col) {
    if (row) {
        *row = term_row;
    }
    if (col) {
        *col = term_col;
    }
}

// --- Coordinate-based Character/String Writes ---

/**
 * @brief Writes a character at a specific position with the current style.
 * Does not move the cursor or trigger scrolling.
 * @param c The character to write.
 * @param row The row to write at (0 to VGA_HEIGHT-1).
 * @param col The column to write at (0 to VGA_WIDTH-1).
 */
void vga_putc_at(char c, int row, int col) {
    vga_put_char_at(c, term_style, row, col);
}

/**
 * @brief Writes a character at a specific position with a specific style.
 * Does not move the cursor or trigger scrolling.
 * @param c The character to write.
 * @param style The style byte to use.
 * @param row The row to write at (0 to VGA_HEIGHT-1).
 * @param col The column to write at (0 to VGA_WIDTH-1).
 */
void vga_putc_at_styled(char c, uint8_t style, int row, int col) {
    vga_put_char_at(c, style, row, col);
}

/**
 * @brief Writes a string at a specific position with the current style.
 * Does not move the cursor or trigger scrolling.
 * Will stop at screen boundaries (no wrapping).
 * @param str The string to write.
 * @param row The row to start writing at (0 to VGA_HEIGHT-1).
 * @param col The column to start writing at (0 to VGA_WIDTH-1).
 */
void vga_puts_at(const char* str, int row, int col) {
    if (!str || row < 0 || row >= VGA_HEIGHT) {
        return;
    }
    int current_col = col;
    for (size_t i = 0; str[i] != '\0' && current_col < VGA_WIDTH; ++i) {
        vga_put_char_at(str[i], term_style, row, current_col);
        current_col++;
    }
}

/**
 * @brief Writes a string at a specific position with a specific style.
 * Does not move the cursor or trigger scrolling.
 * Will stop at screen boundaries (no wrapping).
 * @param str The string to write.
 * @param style The style byte to use.
 * @param row The row to start writing at (0 to VGA_HEIGHT-1).
 * @param col The column to start writing at (0 to VGA_WIDTH-1).
 */
void vga_puts_at_styled(const char* str, uint8_t style, int row, int col) {
    if (!str || row < 0 || row >= VGA_HEIGHT) {
        return;
    }
    int current_col = col;
    for (size_t i = 0; str[i] != '\0' && current_col < VGA_WIDTH; ++i) {
        vga_put_char_at(str[i], style, row, current_col);
        current_col++;
    }
}

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
void vga_fill_rect(int row, int col, int width, int height, char c, uint8_t style) {
    if (row < 0 || col < 0 || width <= 0 || height <= 0) {
        return;
    }
    
    int end_row = row + height;
    int end_col = col + width;
    
    if (end_row > VGA_HEIGHT) {
        end_row = VGA_HEIGHT;
    }
    if (end_col > VGA_WIDTH) {
        end_col = VGA_WIDTH;
    }
    
    for (int y = row; y < end_row; ++y) {
        for (int x = col; x < end_col; ++x) {
            vga_put_char_at(c, style, y, x);
        }
    }
}

/**
 * @brief Clears a rectangular region (fills with spaces) using the current style.
 * @param row The starting row (0 to VGA_HEIGHT-1).
 * @param col The starting column (0 to VGA_WIDTH-1).
 * @param width The width of the rectangle.
 * @param height The height of the rectangle.
 */
void vga_clear_rect(int row, int col, int width, int height) {
    vga_fill_rect(row, col, width, height, ' ', term_style);
}

/**
 * @brief Draws a horizontal line at a specific position.
 * @param row The row to draw at.
 * @param col The starting column.
 * @param length The length of the line.
 * @param c The character to use for the line.
 * @param style The style byte to use.
 */
void vga_draw_hline(int row, int col, int length, char c, uint8_t style) {
    if (row < 0 || row >= VGA_HEIGHT || col < 0 || length <= 0) {
        return;
    }
    
    int end_col = col + length;
    if (end_col > VGA_WIDTH) {
        end_col = VGA_WIDTH;
    }
    
    for (int x = col; x < end_col; ++x) {
        vga_put_char_at(c, style, row, x);
    }
}

/**
 * @brief Draws a vertical line at a specific position.
 * @param row The starting row.
 * @param col The column to draw at.
 * @param length The length of the line.
 * @param c The character to use for the line.
 * @param style The style byte to use.
 */
void vga_draw_vline(int row, int col, int length, char c, uint8_t style) {
    if (row < 0 || col < 0 || col >= VGA_WIDTH || length <= 0) {
        return;
    }
    
    int end_row = row + length;
    if (end_row > VGA_HEIGHT) {
        end_row = VGA_HEIGHT;
    }
    
    for (int y = row; y < end_row; ++y) {
        vga_put_char_at(c, style, y, col);
    }
}

/**
 * @brief Draws a box (border) at a specific position.
 * @param row The starting row.
 * @param col The starting column.
 * @param width The width of the box (including borders).
 * @param height The height of the box (including borders).
 * @param style The style byte to use.
 */
void vga_draw_box(int row, int col, int width, int height, uint8_t style) {
    if (row < 0 || col < 0 || width < 2 || height < 2) {
        return;
    }
    
    int end_row = row + height - 1;
    int end_col = col + width - 1;
    
    if (row >= VGA_HEIGHT || col >= VGA_WIDTH) {
        return;
    }
    
    if (end_row >= VGA_HEIGHT) {
        end_row = VGA_HEIGHT - 1;
    }
    if (end_col >= VGA_WIDTH) {
        end_col = VGA_WIDTH - 1;
    }
    
    // Top and bottom borders
    vga_draw_hline(row, col + 1, width - 2, VGA_BOX_HORIZONTAL, style);
    vga_draw_hline(end_row, col + 1, width - 2, VGA_BOX_HORIZONTAL, style);
    
    // Left and right borders
    vga_draw_vline(row + 1, col, height - 2, VGA_BOX_VERTICAL, style);
    vga_draw_vline(row + 1, end_col, height - 2, VGA_BOX_VERTICAL, style);
    
    // Corners
    vga_put_char_at(VGA_BOX_TOP_LEFT, style, row, col);
    vga_put_char_at(VGA_BOX_TOP_RIGHT, style, row, end_col);
    vga_put_char_at(VGA_BOX_BOTTOM_LEFT, style, end_row, col);
    vga_put_char_at(VGA_BOX_BOTTOM_RIGHT, style, end_row, end_col);
}

// Hardware cursor functions (outb, inb) would be here if implemented.
// void vga_update_cursor(int row, int col) { ... }
// void vga_enable_cursor(uint8_t cursor_start, uint8_t cursor_end) { ... }
// void vga_disable_cursor() { ... }
