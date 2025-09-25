/**
 * @file memory_bug_example.c
 * @brief A C program with a subtle heap corruption bug.
 *
 * This code is designed as a teaching tool for demonstrating debugging with
 * GDB and Valgrind on Linux systems.
 *
 * The Bug:
 * The program allocates a fixed-size buffer for user names. One of the names
 * provided is too long, and the use of `strcpy` causes a buffer overflow.
 * This overflow corrupts the metadata of the *next* memory chunk on the heap.
 * The program does not crash immediately. The crash happens later when
 * `free()` is called on a different, valid pointer, because the memory
 * manager detects the heap corruption.
 *
 * --- How to Compile ---
 * gcc -g -o memory_bug memory_bug_example.c
 *
 * The -g flag is crucial as it includes debugging symbols.
 *
 * --- How to Debug (The Lesson Plan) ---
 * 1. Run ./memory_bug. It will crash with a message like "free(): corrupted size vs. prev_size".
 * 2. Run with GDB: `gdb ./memory_bug`. Type `run`.
 * - GDB will stop at the crash point, inside `free()`.
 * - Use `bt` (backtrace). It will point to line 85 in `main`, where we free
 * the memory for "Charles". This line is innocent. This is the key
 * teaching moment: the crash location is not the bug's location.
 * 3. Run with Valgrind: `valgrind --leak-check=full ./memory_bug`.
 * - Valgrind will immediately report an "Invalid write of size..."
 * and point directly to the `strcpy` on line 46.
 * - It precisely identifies the source buffer ("MontgomeryBurns") and the
 * destination buffer size (16 bytes), explaining the overflow.
 * - This demonstrates the power of Valgrind for finding the true root cause.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NAME_BUFFER_SIZE 16
#define NUM_USERS 3

typedef struct {
    int id;
    char* name;
} User;

/**
 * @brief Allocates memory for a user's name and copies it.
 *
 * This function contains the hidden bug. When a name longer than
 * NAME_BUFFER_SIZE-1 is passed, strcpy will write out of bounds.
 */
void set_user_name(User* user, const char* name) {
    user->name = (char*)malloc(NAME_BUFFER_SIZE);
    if (user->name == NULL) {
        perror("Failed to allocate memory for name");
        exit(EXIT_FAILURE);
    }
    // BUG: No bounds check! If `name` is >= 16 chars, this will overflow.
    strcpy(user->name, name);
    printf("Assigned name '%s' to user %d.\n", name, user->id);
}

/**
 * @brief Prints the details of all users.
 */
void print_user_report(User** users, int count) {
    printf("\n--- User Report ---\n");
    for (int i = 0; i < count; i++) {
        if (users[i] != NULL) {
            printf("User ID: %d, Name: %s\n", users[i]->id, users[i]->name);
        }
    }
    printf("-------------------\n\n");
}

int main() {
    User* user_database[NUM_USERS];
    const char* names[] = {"Alice", "MontgomeryBurns", "Charles"};

    printf("Initializing user database...\n");

    // --- Allocation and Initialization Phase ---
    for (int i = 0; i < NUM_USERS; i++) {
        user_database[i] = (User*)malloc(sizeof(User));
        if (user_database[i] == NULL) {
            perror("Failed to allocate memory for user");
            return EXIT_FAILURE;
        }
        user_database[i]->id = 100 + i;
        // The overflow happens here on the second iteration (i=1)
        set_user_name(user_database[i], names[i]);
    }

    // At this point, the heap is already corrupted, but the program seems fine.
    printf("\nDatabase initialization complete. Everything seems OK.\n");
    print_user_report(user_database, NUM_USERS);

    // --- Cleanup Phase ---
    printf("Deallocating memory...\n");
    for (int i = 0; i < NUM_USERS; i++) {
        if (user_database[i] != NULL) {
            printf("Freeing name for user %d ('%s')...\n", user_database[i]->id, user_database[i]->name);
            free(user_database[i]->name); // The crash does NOT happen here.

            printf("Freeing struct for user %d...\n", user_database[i]->id);
            // The crash is LIKELY to happen on the i=2 iteration here,
            // when free() is called on the memory block whose metadata was
            // corrupted by the overflow from user_database[1]->name.
            free(user_database[i]);
        }
    }

    printf("Memory successfully deallocated.\n"); // This line will not be reached.

    return EXIT_SUCCESS;
}

