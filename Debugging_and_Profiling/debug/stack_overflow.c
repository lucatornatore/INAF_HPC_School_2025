/**
 * @file stack_overflow_example.c
 * @brief A C program designed to cause a stack overflow with large inputs.
 *
 * This code is a teaching tool for demonstrating stack memory limits and
 * debugging with GDB.
 *
 * The Bug:
 * The program uses a deeply recursive function (`recursive_sum`) to calculate
 * the sum of array elements. Each recursive call adds a new frame to the
 * call stack. While this works for small arrays, a sufficiently large array
 * will cause the call stack to exhaust its fixed memory limit, resulting in a
 * stack overflow and a segmentation fault.
 *
 * This demonstrates that code can be logically correct but still fail under
 * certain data conditions.
 *
 * --- How to Compile ---
 * gcc -g -o stack_overflow stack_overflow_example.c
 *
 * --- How to Debug (The Lesson Plan) ---
 * 1. Run with a small, safe size (default):
 * ./stack_overflow
 * It will run successfully.
 *
 * 2. Run with a large size that causes a crash:
 * ./stack_overflow 500000
 * (The exact number to crash may vary by system, but this is a likely candidate)
 * It will crash with "Segmentation fault".
 *
 * 3. Debug with GDB:
 * gdb ./stack_overflow
 * (gdb) run 500000
 * The program will crash.
 * (gdb) bt
 * The `bt` (backtrace) command will show thousands of nested calls to
 * `recursive_sum`, visually demonstrating that the stack is full.
 * GDB may even stop printing the backtrace and say "Backtrace stopped:
 * previous frame identical to this frame". This is the smoking gun for
 * runaway recursion.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// A default "safe" size that will not cause a crash.
#define DEFAULT_SIZE 10000

/**
 * @brief Recursively calculates the sum of an array.
 *
 * This function's call depth is equal to the number of elements it needs to sum.
 * For a large `count`, this will exhaust the stack.
 *
 * @param array The array of integers.
 * @param count The number of elements remaining to be summed.
 * @return The sum of the elements.
 */
long long recursive_sum(int* array, size_t count) {
    // Base case: If there are no elements left, the sum is 0.
    if (count == 0) {
        return 0;
    }
    // Recursive step: Return the first element plus the sum of the rest.
    return array[0] + recursive_sum(array + 1, count - 1);
}

int main(int argc, char* argv[]) {
    size_t size = DEFAULT_SIZE;

    if (argc > 1) {
        size = atol(argv[1]);
        if (size == 0) {
            fprintf(stderr, "Invalid size provided. Using default.\n");
            size = DEFAULT_SIZE;
        }
    }

    printf("Attempting to sum an array of %zu integers using recursion...\n", size);

    int* numbers = (int*)malloc(size * sizeof(int));
    if (numbers == NULL) {
        perror("Failed to allocate memory for the array");
        return EXIT_FAILURE;
    }

    // Initialize the array with some values.
    for (size_t i = 0; i < size; i++) {
        numbers[i] = 1;
    }

    // This call will trigger the stack overflow if `size` is large enough.
    long long sum = recursive_sum(numbers, size);

    printf("Sum calculated successfully: %lld\n", sum);

    free(numbers);
    return EXIT_SUCCESS;
}

