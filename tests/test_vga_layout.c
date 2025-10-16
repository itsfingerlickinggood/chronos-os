#include "kernel/vga.h"
#include "kernel/printf.h"

void test_vga_layout_primitives(void) {
    uint8_t default_style = vga_entry_style(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    uint8_t header_style = vga_entry_style(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    uint8_t panel_style = vga_entry_style(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);

    vga_clear_screen();
    
    vga_draw_box(0, 0, VGA_WIDTH, 3, header_style);
    vga_puts_at_styled("  System Dashboard", header_style, 1, 1);
    
    vga_draw_box(4, 0, 40, 10, default_style);
    vga_puts_at_styled("CPU Info Panel", panel_style, 5, 2);
    vga_puts_at("CPU: x86", 6, 3);
    vga_puts_at("Load: 25%", 7, 3);
    
    vga_draw_box(4, 41, 39, 10, default_style);
    vga_puts_at_styled("Memory Panel", panel_style, 5, 43);
    vga_puts_at("Total: 64MB", 6, 44);
    vga_puts_at("Free: 48MB", 7, 44);
    
    vga_draw_box(15, 0, VGA_WIDTH, 10, default_style);
    vga_puts_at_styled("System Log", panel_style, 16, 2);
    vga_puts_at("> Kernel initialized", 17, 2);
    vga_puts_at("> VGA driver loaded", 18, 2);
    vga_puts_at("> Layout test complete", 19, 2);
    
    int old_row, old_col;
    vga_get_cursor_pos(&old_row, &old_col);
    vga_set_cursor_pos(23, 0);
    kprintf("Cursor moved from (%d,%d) to (23,0)\n", old_row, old_col);
}
