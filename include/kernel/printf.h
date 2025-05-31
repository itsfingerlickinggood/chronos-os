#ifndef _KERNEL_PRINTF_H
#define _KERNEL_PRINTF_H

#include <stdarg.h> // For va_list, va_start, va_arg, va_end

/**
 * @brief Kernel-level printf function.
 *
 * Outputs a formatted string to the VGA display.
 * Supports basic format specifiers:
 *   %c - character
 *   %s - string
 *   %d - signed integer (decimal)
 *   %x - unsigned integer (hexadecimal, lowercase)
 *   %p - pointer (hexadecimal, lowercase, prefixed with "0x")
 *   %% - literal '%'
 *
 * @param format The format string.
 * @param ...    Variable arguments corresponding to format specifiers.
 * @return int   The number of characters written.
 */
int kprintf(const char* format, ...);

#endif // _KERNEL_PRINTF_H
