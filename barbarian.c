// Including necessary headers for the functionality
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <string.h>  // For handling string operations like strlen
#include <fcntl.h>   // For file control options
#include <sys/mman.h> // For memory management functions
#include "dungeon_info.h"
#include "dungeon_settings.h"

// Global variable declarations for easy access throughout the program
struct Dungeon *dungeon; // Pointer to the shared Dungeon structure
sem_t *lever; // Semaphore used for coordinating access to the treasure

// Function to handle signals specific to Barbarian
void Barbarian_signal_handler(int signal, struct Dungeon *dungeon) {
    if (signal == DUNGEON_SIGNAL) {
        // Copy enemy health to barbarian's attack field when a signal is received
        dungeon->barbarian.attack = dungeon->enemy.health;
        // Check if the copy was successful and print the result
        if (dungeon->barbarian.attack == dungeon->enemy.health) {
            printf("Success on attack\n");
        } else {
            printf("Failed on attack\n");
        }
        // Delay to simulate time taken for an attack
        sleep(SECONDS_TO_ATTACK);
    }
    else if (signal == SEMAPHORE_SIGNAL) {
        // Wait on the semaphore when told to hold the door in the treasure room
        sem_wait(lever);
        printf("Barbarian is holding the lever.\n");
        // Hold the semaphore until all treasure is collected
        while (strlen(dungeon->spoils) < 4);
        // Hold the lever for a predefined time after all treasure is collected
        sleep(TIME_TREASURE_AVAILABLE);
        // Release the semaphore to open the door
        sem_post(lever);
        printf("Barbarian has released the lever.\n");
    }
}

// Signal handler setup to properly handle signals with additional information
void Actual_Barbarian_Handler(int signal, siginfo_t *info, void *ucontext) {
    Barbarian_signal_handler(signal, dungeon); // Delegate to the main handler function
}

int main() {
    // Open the shared memory segment
    int shm_fd = shm_open(dungeon_shm_name, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Failed to open shared memory");
        exit(EXIT_FAILURE);
    }

    // Map the shared memory to access the Dungeon structure
    dungeon = mmap(NULL, sizeof(struct Dungeon), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (dungeon == MAP_FAILED) {
        perror("Failed to mmap shared memory");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    // Open or create the semaphore associated with the first lever
    lever = sem_open(dungeon_lever_one, O_CREAT, 0666, 0); 
    if (lever == SEM_FAILED) {
        perror("Failed to open semaphore for lever one in barbarian");
        exit(EXIT_FAILURE);
    }

    // Setup signal handling for both dungeon and semaphore signals
    struct sigaction sa;
    sa.sa_sigaction = Actual_Barbarian_Handler; // Specify the handler function with siginfo
    sigemptyset(&sa.sa_mask); // Initialize signal mask to empty, meaning no signals are blocked
    sa.sa_flags = SA_SIGINFO; // Enable use of the sa_sigaction field instead of sa_handler

    // Apply the configured signal actions
    if (sigaction(DUNGEON_SIGNAL, &sa, NULL) != 0 || sigaction(SEMAPHORE_SIGNAL, &sa, NULL) != 0) {
        perror("Sigaction failed");
        exit(EXIT_FAILURE);
    }

    // Notify that the Barbarian is running and waiting for signals
    printf("Barbarian is running, waiting on signal.\n");
    while (dungeon->running) {
        pause(); // Wait indefinitely for signals to process
    }

    // Print when Barbarian has finished its task
    printf("Barbarian is done with its task\n");
    return 0;
}
