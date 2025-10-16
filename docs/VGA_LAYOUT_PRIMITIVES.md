# VGA Layout Primitives

This document describes the new VGA text-mode layout primitives added to support dashboard and UI panel rendering.

## Overview

The VGA driver has been extended with coordinate-based drawing functions, cursor control, and region fill helpers that enable structured layout creation without affecting normal terminal scrolling behavior.

## New APIs

### Cursor Control

#### `vga_set_cursor_pos(int row, int col)`
Sets the cursor position for subsequent `vga_putc()` and `vga_puts()` calls.
- **Parameters:**
  - `row`: Row position (0 to 24)
  - `col`: Column position (0 to 79)
- **Use case:** Repositioning output location without affecting existing screen content

#### `vga_get_cursor_pos(int* row, int* col)`
Retrieves the current cursor position.
- **Parameters:**
  - `row`: Pointer to store current row (can be NULL)
  - `col`: Pointer to store current column (can be NULL)
- **Use case:** Saving cursor position before drawing UI elements, then restoring it

### Coordinate-based Character/String Writes

#### `vga_putc_at(char c, int row, int col)`
Writes a single character at a specific position using the current style.
- **Does not** move the cursor
- **Does not** trigger scrolling
- **Use case:** Drawing grid markers, status indicators

#### `vga_putc_at_styled(char c, uint8_t style, int row, int col)`
Writes a single character with a specific style at a position.
- **Does not** move the cursor or trigger scrolling
- **Use case:** Color-coded status indicators

#### `vga_puts_at(const char* str, int row, int col)`
Writes a string starting at a specific position using current style.
- Stops at screen boundaries (no wrapping)
- **Does not** move the cursor or trigger scrolling
- **Use case:** Panel labels, fixed position text

#### `vga_puts_at_styled(const char* str, uint8_t style, int row, int col)`
Writes a string with a specific style at a position.
- Same behavior as `vga_puts_at()` but with custom styling
- **Use case:** Colored panel headers, highlighted text

### Region Fill Helpers

#### `vga_fill_rect(int row, int col, int width, int height, char c, uint8_t style)`
Fills a rectangular region with a specific character and style.
- **Parameters:**
  - `row`, `col`: Top-left corner
  - `width`, `height`: Dimensions in characters
  - `c`: Fill character
  - `style`: Style byte
- **Use case:** Background fills, separator bars

#### `vga_clear_rect(int row, int col, int width, int height)`
Clears a rectangular region (fills with spaces) using current style.
- **Use case:** Clearing panel contents without affecting surrounding UI

#### `vga_draw_hline(int row, int col, int length, char c, uint8_t style)`
Draws a horizontal line.
- **Use case:** Panel separators, progress bars

#### `vga_draw_vline(int row, int col, int length, char c, uint8_t style)`
Draws a vertical line.
- **Use case:** Column dividers, panel borders

#### `vga_draw_box(int row, int col, int width, int height, uint8_t style)`
Draws a box border using ASCII characters.
- Uses `+` for corners, `-` for horizontal, `|` for vertical
- **Use case:** Panel frames, UI containers

## Box Drawing Characters

For monochrome-friendly rendering, the following constants are defined:

```c
#define VGA_BOX_HORIZONTAL '-'
#define VGA_BOX_VERTICAL '|'
#define VGA_BOX_TOP_LEFT '+'
#define VGA_BOX_TOP_RIGHT '+'
#define VGA_BOX_BOTTOM_LEFT '+'
#define VGA_BOX_BOTTOM_RIGHT '+'
#define VGA_BOX_CROSS '+'
```

These use basic ASCII characters that render consistently on all displays.

## Usage Example

```c
// Create a simple dashboard
uint8_t header_style = vga_entry_style(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
uint8_t panel_style = vga_entry_style(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);

// Draw header box
vga_draw_box(0, 0, VGA_WIDTH, 3, header_style);
vga_puts_at_styled("System Dashboard", header_style, 1, 30);

// Draw CPU panel
vga_draw_box(4, 0, 40, 10, panel_style);
vga_puts_at("CPU Info", 5, 2);
vga_puts_at("Load: 25%", 6, 2);

// Draw memory panel
vga_draw_box(4, 41, 39, 10, panel_style);
vga_puts_at("Memory Info", 5, 43);
vga_puts_at("Free: 48MB", 6, 43);

// Normal terminal output continues below panels
vga_set_cursor_pos(15, 0);
kprintf("System log messages appear here...\n");
```

## Design Principles

1. **Non-invasive**: Layout primitives don't affect cursor position or trigger scrolling
2. **Boundary-safe**: All functions handle out-of-bounds coordinates gracefully
3. **Style-aware**: Functions respect current style or accept explicit style parameters
4. **Composable**: Primitives can be combined to create complex layouts
5. **Monochrome-friendly**: Box drawing uses ASCII for maximum compatibility

## Compatibility

- All new functions preserve existing `vga_putc()` and `vga_puts()` behavior
- Terminal scrolling and line wrapping continue to work as before
- Existing kernel code requires no modifications

## Performance

- Direct VGA buffer writes (no intermediate buffering)
- Boundary checks prevent invalid memory access
- Minimal overhead per character operation
