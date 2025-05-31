#include "kernel/memory.h"
#include <stddef.h> // For size_t and NULL

// --- Static global variables for the memory allocator ---

// The actual memory pool for the kernel.
// Defined in include/kernel/memory.h: KERNEL_MEMORY_SIZE
static char kernel_memory_area[KERNEL_MEMORY_SIZE];

// Bitmap to track the status of memory blocks.
// Each bit represents a block: 0 for free, 1 for used.
// Defined in include/kernel/memory.h: MAX_BLOCKS
// Size is MAX_BLOCKS / 8 because each char (byte) can hold 8 bits.
static char memory_bitmap[MAX_BLOCKS / 8];

// --- Memory Management Functions ---

/**
 * @brief Initializes the kernel memory manager.
 *
 * This function should be called once at kernel startup.
 * It clears the memory_bitmap, marking all memory blocks as free.
 */
void memory_init(void) {
    for (size_t i = 0; i < (MAX_BLOCKS / 8); ++i) {
        memory_bitmap[i] = 0;
    }
    // If MAX_BLOCKS is not a multiple of 8, the last few bits in the
    // last byte of the bitmap also need to be considered if a more
    // precise initialization were required, but for now, zeroing out
    // the bytes is sufficient as unused bits will remain 0 (free).
}

/**
 * @brief Allocates a single block of memory.
 *
 * Allocates a memory block of BLOCK_SIZE. This is a very simple allocator.
 * It does not support allocating sizes other than BLOCK_SIZE.
 *
 * @param size The size of memory to allocate. Currently, this parameter is
 *             checked: if it's 0 or greater than BLOCK_SIZE, NULL is returned.
 *             Otherwise, a block of BLOCK_SIZE is allocated.
 * @return void* Pointer to the allocated memory block, or NULL if allocation fails
 *               (no free block or invalid size).
 */
void* kmalloc(size_t size) {
    // Basic checks for the requested size.
    // This simple allocator only allocates fixed-size blocks.
    if (size == 0 || size > BLOCK_SIZE) {
        return NULL; // Cannot allocate zero size or more than a single block.
    }

    // Iterate through the bitmap to find a free block.
    for (int block_idx = 0; block_idx < MAX_BLOCKS; ++block_idx) {
        // Calculate the byte and bit in the bitmap for the current block.
        int byte_idx = block_idx / 8;
        int bit_offset = block_idx % 8;

        // Check if the bit is 0 (free).
        if (!(memory_bitmap[byte_idx] & (1 << bit_offset))) {
            // Mark the block as used (set the bit to 1).
            memory_bitmap[byte_idx] |= (1 << bit_offset);

            // Calculate the address of the allocated block.
            void* block_address = (void*)(kernel_memory_area + (block_idx * BLOCK_SIZE));
            return block_address;
        }
    }

    return NULL; // No free block found.
}

/**
 * @brief Frees a previously allocated memory block.
 *
 * (Placeholder implementation)
 * This function should mark the block corresponding to `ptr` as free in the bitmap.
 *
 * @param ptr Pointer to the memory block to free.
 */
void kfree(void* ptr) {
    // 1. Basic validation: if ptr is NULL, return immediately.
    if (ptr == NULL) {
        // Optionally, log this attempt to free a NULL pointer.
        return;
    }

    // Convert ptr to char* for arithmetic.
    char* p_char = (char*)ptr;

    // 2. Validate pointer range: check if ptr is within kernel_memory_area.
    if (p_char < kernel_memory_area || p_char >= (kernel_memory_area + KERNEL_MEMORY_SIZE)) {
        // Optionally, log this error: pointer out of bounds.
        // For a real kernel, this might trigger a panic.
        return;
    }

    // 3. Validate pointer alignment: check if ptr is aligned to BLOCK_SIZE.
    if (((p_char - kernel_memory_area) % BLOCK_SIZE) != 0) {
        // Optionally, log this error: pointer not block-aligned.
        // For a real kernel, this might trigger a panic.
        return;
    }

    // 4. Calculate block index.
    int block_idx = (p_char - kernel_memory_area) / BLOCK_SIZE;

    // This check is somewhat redundant if range and alignment checks passed,
    // but good for safety. block_idx should be < MAX_BLOCKS.
    // If block_idx were >= MAX_BLOCKS, the range check p_char < (kernel_memory_area + KERNEL_MEMORY_SIZE)
    // should have caught it, assuming KERNEL_MEMORY_SIZE is a multiple of BLOCK_SIZE.
    if (block_idx < 0 || block_idx >= MAX_BLOCKS) {
        // Should not happen if previous checks are correct.
        // Log error or panic.
        return;
    }

    // Calculate the byte and bit in the bitmap for the block.
    int byte_idx = block_idx / 8;
    int bit_offset = block_idx % 8;

    // 5. Check if the block was actually allocated.
    // If the bit is 0, it means the block was already free. This could indicate a double free.
    if (!(memory_bitmap[byte_idx] & (1 << bit_offset))) {
        // Optionally, log this error: attempt to free an already free block (double free).
        // For a real kernel, this might trigger a panic.
        return;
    }

    // Mark the block as free (clear the bit to 0).
    memory_bitmap[byte_idx] &= ~(1 << bit_offset);
}
