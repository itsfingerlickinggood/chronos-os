#include "kernel/memory.h"
#include "kernel/printf.h" // For kprintf
#include <stddef.h> // For NULL

// Use kprintf for test reporting
#define test_printf kprintf

void run_memory_tests(void) {
    void* block1;
    void* blocks[MAX_BLOCKS];
    int tests_passed = 0;
    int tests_failed = 0;

    test_printf("--- Running Memory Manager Tests ---\n");

    // Test 1: Allocate a single block
    test_printf("Test 1: Allocate a single block (100 bytes)...\n");
    block1 = kmalloc(100);
    if (block1 != NULL) {
        test_printf("  SUCCESS: kmalloc returned a valid pointer.\n");
        tests_passed++;
    } else {
        test_printf("  FAILURE: kmalloc returned NULL.\n");
        tests_failed++;
    }

    // Test 2: Free the allocated block
    test_printf("Test 2: Free the allocated block...\n");
    if (block1 != NULL) { // Only try to free if allocation succeeded
        kfree(block1);
        // No direct verification here other than not crashing.
        // A subsequent allocation will implicitly test if it became available.
        test_printf("  SUCCESS: kfree called (visual inspection for crashes).\n");
        tests_passed++;
    } else {
        test_printf("  SKIPPED: Block1 was NULL, cannot free.\n");
    }

    // Test 2.1: Attempt to re-allocate the freed block
    test_printf("Test 2.1: Attempt to re-allocate the previously freed block...\n");
    void* block1_realloc = kmalloc(100);
    if (block1_realloc != NULL) {
        test_printf("  SUCCESS: Re-allocation after free succeeded.\n");
        tests_passed++;
        kfree(block1_realloc); // Clean up
    } else {
        test_printf("  FAILURE: Re-allocation after free failed.\n");
        tests_failed++;
    }


    // Test 3: Allocate all available blocks
    test_printf("Test 3: Allocate all available blocks (%d blocks)...\n", MAX_BLOCKS);
    int allocated_count = 0;
    for (int i = 0; i < MAX_BLOCKS; ++i) {
        blocks[i] = kmalloc(1); // Allocate minimal size for each block
        if (blocks[i] != NULL) {
            allocated_count++;
        } else {
            test_printf("  FAILURE: kmalloc returned NULL prematurely at block %d.\n", i);
            tests_failed++;
            break; // Stop if allocation fails
        }
    }
    if (allocated_count == MAX_BLOCKS) {
        test_printf("  SUCCESS: Allocated all %d blocks.\n", MAX_BLOCKS);
        tests_passed++;
    } else {
        test_printf("  FAILURE: Only allocated %d out of %d blocks.\n", allocated_count, MAX_BLOCKS);
        // This path might be taken if a previous test failed to free memory
    }

    test_printf("Test 3.1: Attempt to allocate one more block (should fail)...\n");
    void* extra_block = kmalloc(1);
    if (extra_block == NULL) {
        test_printf("  SUCCESS: kmalloc returned NULL as expected (no more memory).\n");
        tests_passed++;
    } else {
        test_printf("  FAILURE: kmalloc allocated an extra block when it shouldn't have.\n");
        tests_failed++;
        if (extra_block != NULL) kfree(extra_block); // Clean up if allocated
    }

    // Test 4: Free all allocated blocks
    test_printf("Test 4: Free all %d allocated blocks...\n", allocated_count);
    for (int i = 0; i < allocated_count; ++i) {
        if (blocks[i] != NULL) {
            kfree(blocks[i]);
        }
    }
    // No direct verification other than not crashing.
    test_printf("  SUCCESS: kfree called for all blocks (visual inspection for crashes).\n");
    tests_passed++;


    // Test 5: Re-allocate a block after freeing all
    test_printf("Test 5: Re-allocate a block after freeing all...\n");
    void* block_after_full_free = kmalloc(100);
    if (block_after_full_free != NULL) {
        test_printf("  SUCCESS: kmalloc returned a valid pointer.\n");
        tests_passed++;
        kfree(block_after_full_free); // Clean up
    } else {
        test_printf("  FAILURE: kmalloc returned NULL after freeing all blocks.\n");
        tests_failed++;
    }

    // Test 6: Test freeing a NULL pointer
    test_printf("Test 6: Free a NULL pointer...\n");
    kfree(NULL); // Should not crash
    test_printf("  SUCCESS: kfree(NULL) called (visual inspection for crashes).\n");
    tests_passed++;

    // Test 7: Test freeing an invalid (out of range, low) pointer
    test_printf("Test 7: Free an invalid pointer (out of range, low)...\n");
    char* invalid_low_ptr = (char*)0x100; // Arbitrary low address
    kfree((void*)invalid_low_ptr);
    test_printf("  SUCCESS: kfree((void*)0x100) called (visual inspection for crashes, should be handled gracefully).\n");
    tests_passed++;

    // Test 8: Test freeing an invalid (out of range, high) pointer
    test_printf("Test 8: Free an invalid pointer (out of range, high)...\n");
    // Assuming kernel_memory_area is somewhat low, this should be out of bounds high
    // This test requires kernel_memory_area to be visible or a known address.
    // For now, we cannot directly calculate invalid_high_ptr without knowing kernel_memory_area.
    // kprintf("  SKIPPING Test 8: Free an invalid pointer (out of range, high) - requires knowledge of kernel_memory_area address.\n");
    // Instead, let's test freeing a pointer that is just beyond a valid allocation, if possible,
    // but that's also tricky. For now, this specific high-pointer test is hard to make portable here.
    // Let's comment it out or simplify if possible. The low pointer test is more straightforward.
    // char* invalid_high_ptr = (char*)( ( (char*)kernel_memory_area ) + KERNEL_MEMORY_SIZE + BLOCK_SIZE );
    // kfree((void*)invalid_high_ptr);
    // test_printf("  SUCCESS: kfree(invalid_high_ptr) called (visual inspection for crashes, should be handled gracefully).\n");
    // tests_passed++;
    test_printf("  Test 8: (Skipped) Free an invalid pointer (out of range, high) - needs direct memory layout info.\n");


    // Test 9: Test freeing a misaligned pointer
    test_printf("Test 9: Free a misaligned pointer...\n");
    void* temp_block_for_misalign = kmalloc(1);
    if (temp_block_for_misalign != NULL) {
        char* misaligned_ptr = (char*)temp_block_for_misalign + 1;
        kfree((void*)misaligned_ptr);
        test_printf("  SUCCESS: kfree(misaligned_ptr) called (visual inspection for crashes, should be handled gracefully).\n");
        tests_passed++;
        kfree(temp_block_for_misalign); // Free the original block
    } else {
        test_printf("  SKIPPED: Could not allocate block for misalignment test.\n");
    }

    // Test 10: Double free test
    test_printf("Test 10: Double free a block...\n");
    void* double_free_block = kmalloc(1);
    if (double_free_block != NULL) {
        kfree(double_free_block);
        kfree(double_free_block); // Second free
        test_printf("  SUCCESS: kfree() called twice on the same block (visual inspection, should be handled gracefully).\n");
        tests_passed++;
        // Attempt to allocate again to see if the pool is corrupted (it shouldn't be if double free is handled)
        void* after_double_free = kmalloc(1);
        if (after_double_free != NULL) {
            test_printf("  POST-DOUBLE-FREE CHECK: Allocation successful.\n");
            kfree(after_double_free);
        } else {
            test_printf("  POST-DOUBLE-FREE CHECK: Allocation failed. Memory pool might be corrupted.\n");
            tests_failed++;
        }
    } else {
        test_printf("  SKIPPED: Could not allocate block for double free test.\n");
    }


    test_printf("--- Memory Manager Test Summary ---\n");
    test_printf("Tests Passed: %d\n", tests_passed);
    test_printf("Tests Failed: %d\n", tests_failed);
    test_printf("-----------------------------------\n");
}
