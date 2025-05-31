#include "kernel/printf.h"
#include "kernel/vga.h"   // For vga_putc, vga_puts
#include <stdarg.h>       // For va_list, etc.
#include <stddef.h>       // For NULL
#include <stdint.h>       // For uintptr_t

// --- Static Helper Functions for Number to String Conversion ---

/**
 * @brief Reverses a string in place.
 * @param str String to reverse.
 * @param length Length of the string.
 */
static void k_reverse(char* str, int length) {
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

/**
 * @brief Converts a signed integer to a string (supports base 10).
 * @param value The integer value to convert.
 * @param str Buffer to store the resulting string.
 * @param base The numerical base (only base 10 is fully supported with sign).
 * @return char* Pointer to the beginning of the string in the buffer.
 */
static char* k_itoa(int value, char* str, int base) {
    int i = 0;
    int is_negative = 0;

    // Handle 0 explicitly, otherwise empty string might be returned
    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    if (value < 0 && base == 10) {
        is_negative = 1;
        value = -value; // Make value positive for conversion logic
    }

    // Process individual digits
    while (value != 0) {
        int rem = value % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0'; // For bases > 10
        value = value / base;
    }

    if (is_negative && base == 10) {
        str[i++] = '-';
    }

    str[i] = '\0'; // Null-terminate the string
    k_reverse(str, i); // Reverse the string to get correct order
    return str;
}

/**
 * @brief Converts an unsigned integer to a hexadecimal string.
 * @param value The unsigned integer value.
 * @param str Buffer to store the resulting hex string.
 * @return char* Pointer to the beginning of the string in the buffer.
 */
static char* k_uitoa_hex(unsigned int value, char* str) {
    int i = 0;
    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }
    while (value != 0) {
        unsigned int rem = value % 16;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0'; // Use 'a'-'f' for hex
        value = value / 16;
    }
    str[i] = '\0';
    k_reverse(str, i);
    return str;
}


// --- kprintf Implementation ---

int kprintf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    char buffer[32]; // Buffer for number-to-string conversions (e.g., max 32-bit int in decimal + sign)
    int chars_written = 0;

    if (!format) {
        va_end(args);
        return 0;
    }

    for (int i = 0; format[i] != '\0'; ++i) {
        if (format[i] == '%') {
            i++; // Move to the character after '%'
            switch (format[i]) {
                case '\0': // Dangling '%' at end of string
                    vga_putc('%');
                    chars_written++;
                    goto end_loop; // Break out of the for loop
                case 'c': {
                    char c = (char)va_arg(args, int); // char promotes to int in varargs
                    vga_putc(c);
                    chars_written++;
                    break;
                }
                case 's': {
                    const char* s = va_arg(args, const char*);
                    if (!s) {
                        s = "(null)";
                    }
                    vga_puts(s);
                    // Estimate characters written for strings
                    const char* temp_s = s;
                    while(*temp_s++) chars_written++;
                    break;
                }
                case 'd': {
                    int d = va_arg(args, int);
                    k_itoa(d, buffer, 10);
                    vga_puts(buffer);
                    char* temp_b = buffer;
                    while(*temp_b++) chars_written++;
                    break;
                }
                case 'x': { // Lowercase hex
                    unsigned int x_val = va_arg(args, unsigned int);
                    k_uitoa_hex(x_val, buffer);
                    // No "0x" prefix by default for %x, common stdio behavior
                    vga_puts(buffer);
                    char* temp_b = buffer;
                    while(*temp_b++) chars_written++;
                    break;
                }
                case 'p': { // Pointer (print as hex, prefixed with "0x")
                    // uintptr_t is from <stdint.h>, ensures void* can be cast to unsigned int
                    uintptr_t p_val = (uintptr_t)va_arg(args, void*);
                    k_uitoa_hex((unsigned int)p_val, buffer); // Cast to unsigned int for k_uitoa_hex
                    vga_puts("0x");
                    chars_written += 2;
                    vga_puts(buffer);
                    char* temp_b = buffer;
                    while(*temp_b++) chars_written++;
                    break;
                }
                case '%': {
                    vga_putc('%');
                    chars_written++;
                    break;
                }
                default: // Unknown format specifier, print it literally
                    vga_putc('%');
                    vga_putc(format[i]);
                    chars_written += 2;
                    break;
            }
        } else {
            vga_putc(format[i]);
            chars_written++;
        }
    }

end_loop:
    va_end(args);
    return chars_written;
}
