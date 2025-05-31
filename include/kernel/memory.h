#ifndef KERNEL_MEMORY_H
#define KERNEL_MEMORY_H

#include <stddef.h> // For size_t

// Memory layout constants
#define KERNEL_MEMORY_SIZE (1024 * 1024) // 1MB of kernel memory
#define BLOCK_SIZE 4096                  // 4KB per block
#define MAX_BLOCKS (KERNEL_MEMORY_SIZE / BLOCK_SIZE)

// Function declarations for memory management
void memory_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);

// Test function for memory manager
void run_memory_tests(void);

#endif // KERNEL_MEMORY_H
