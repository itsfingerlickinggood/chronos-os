# VGA Layout Primitives - Change Log

## Summary

Extended the VGA text-mode driver with layout primitives to support dashboard and panel-based UI rendering while maintaining backward compatibility with existing terminal output functionality.

## Changes

### Header File: `include/kernel/vga.h`

**Added Cursor Control APIs:**
- `vga_set_cursor_pos(int row, int col)` - Set cursor position
- `vga_get_cursor_pos(int* row, int* col)` - Get cursor position

**Added Coordinate-based Write APIs:**
- `vga_putc_at(char c, int row, int col)` - Write character at position (current style)
- `vga_putc_at_styled(char c, uint8_t style, int row, int col)` - Write styled character at position
- `vga_puts_at(const char* str, int row, int col)` - Write string at position (current style)
- `vga_puts_at_styled(const char* str, uint8_t style, int row, int col)` - Write styled string at position

**Added Region Fill APIs:**
- `vga_fill_rect(int row, int col, int width, int height, char c, uint8_t style)` - Fill rectangle
- `vga_clear_rect(int row, int col, int width, int height)` - Clear rectangle
- `vga_draw_hline(int row, int col, int length, char c, uint8_t style)` - Draw horizontal line
- `vga_draw_vline(int row, int col, int length, char c, uint8_t style)` - Draw vertical line
- `vga_draw_box(int row, int col, int width, int height, uint8_t style)` - Draw box border

**Added Box Drawing Constants:**
- `VGA_BOX_HORIZONTAL` - Horizontal line character (`-`)
- `VGA_BOX_VERTICAL` - Vertical line character (`|`)
- `VGA_BOX_TOP_LEFT` - Top-left corner (`+`)
- `VGA_BOX_TOP_RIGHT` - Top-right corner (`+`)
- `VGA_BOX_BOTTOM_LEFT` - Bottom-left corner (`+`)
- `VGA_BOX_BOTTOM_RIGHT` - Bottom-right corner (`+`)
- `VGA_BOX_CROSS` - Cross character (`+`)

### Implementation File: `kernel/vga.c`

**Implemented all new APIs with the following characteristics:**
- All coordinate-based writes preserve cursor position and don't trigger scrolling
- Boundary checking on all operations to prevent invalid memory access
- Reuses existing internal VGA buffer (`term_buffer`) and helper function (`vga_put_char_at`)
- Zero impact on existing `vga_putc()` and `vga_puts()` behavior

## Testing

Created `tests/test_vga_layout.c` demonstrating:
- Dashboard layout with multiple panels
- Box drawing for UI containers
- Mixed coordinate-based and cursor-based output
- Style-aware text rendering

## Documentation

Added `docs/VGA_LAYOUT_PRIMITIVES.md` with:
- Complete API reference
- Usage examples
- Design principles
- Performance characteristics

## Backward Compatibility

✅ **Fully backward compatible:**
- Existing `vga_init()`, `vga_putc()`, `vga_puts()`, `vga_clear_screen()` unchanged
- Terminal scrolling and line wrapping behavior preserved
- No changes required to existing kernel code

## Compilation

✅ All changes compile cleanly with `-Wall -Wextra -Werror`
✅ No warnings or errors introduced
✅ Successfully tested with GCC

## Use Cases

The new primitives enable:
- System monitoring dashboards
- Multi-panel layouts
- Status bars and progress indicators
- Grid-based UIs
- Non-scrolling informational displays
- Mixed static UI with scrolling log output
