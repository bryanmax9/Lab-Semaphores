// Including necessary headers for functionalities such as I/O, system calls, shared memory, and semaphores
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "dungeon_info.h"
#include "dungeon_settings.h"

// Global declaration of the shared dungeon structure for easy access throughout the program
struct Dungeon *dungeon;
sem_t *lever; // Semaphore handle

// Function to handle collecting treasure when a semaphore signal is received
void collect_treasure(int signal) {
    if (signal == SEMAPHORE_SIGNAL) {
        sem_wait(lever); // Wait to acquire the semaphore before accessing the treasure
        for (int i = 0; i < 4; i++) {
            while (dungeon->treasure[i] == '\0') { // Wait until the treasure is available
                sleep(1);
            }
            dungeon->spoils[i] = dungeon->treasure[i]; // Collect the treasure
        }
        sem_post(lever); // Release the semaphore after collecting
    }
}

// Function to simulate lock picking by the rogue
void rogue_pick_lock(int signal, struct Dungeon *dungeon) {
    if (signal == DUNGEON_SIGNAL) {
        sem_wait(lever); // Wait to acquire the semaphore before picking the lock
        float low = 0.0, high = MAX_PICK_ANGLE, mid = 0;
        while (low <= high) {
            mid = (low + high) / 2; // Calculate mid point
            dungeon->rogue.pick = mid; // Set the pick's position
            dungeon->trap.direction = 't'; // Temporary direction to indicate checking
            usleep(TIME_BETWEEN_ROGUE_TICKS); // Pause briefly between checks

            // Adjust the range based on the direction feedback from the trap
            if (dungeon->trap.direction == 'u') {
                low = mid + LOCK_THRESHOLD;
            } else if (dungeon->trap.direction == 'd') {
                high = mid - LOCK_THRESHOLD;
            } else if (dungeon->trap.direction == '-') {
                printf("Lock picked successfully\n");
                break; // Exit the loop if lock is picked
            }
        }

        if (!dungeon->trap.locked) {
            printf("Rogue has successfully unlocked the trap.\n");
        } else {
            printf("Rogue pick failed\n");
        }
        sem_post(lever); // Release the semaphore after operation
    }
}

// Signal handler function to direct to appropriate functions based on the signal type
void rogue_signal_handler(int sig, siginfo_t *info, void *ucontext) {
    if (sig == DUNGEON_SIGNAL) {
        rogue_pick_lock(sig, dungeon); // Handle lock picking
    } else if (sig == SEMAPHORE_SIGNAL) {
        collect_treasure(sig); // Handle treasure collection
    }
}

int main() {
    // Open the shared memory segment
    int shm_fd = shm_open(dungeon_shm_name, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Failed to open shared memory");
        exit(EXIT_FAILURE);
    }

    // Map the shared memory to the Dungeon structure
    dungeon = mmap(NULL, sizeof(struct Dungeon), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (dungeon == MAP_FAILED) {
        perror("Failed to mmap shared memory");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    // Open or create the semaphore for the lever (assuming Rogue uses LeverOne)
    lever = sem_open(dungeon_lever_one, O_CREAT, 0666, 0); 
    if (lever == SEM_FAILED) {
        perror("Failed to open semaphore for lever one in rogue");
        exit(EXIT_FAILURE);
    }

    // Setup signal handling
    struct sigaction sa;
    sa.sa_sigaction = rogue_signal_handler; // Specify the signal handling function
    sigemptyset(&sa.sa_mask); // Initialize the signal set to be empty, so no signals are blocked
    sa.sa_flags = SA_SIGINFO; // Use the sa_sigaction field
    sigaction(DUNGEON_SIGNAL, &sa, NULL);
    sigaction(SEMAPHORE_SIGNAL, &sa, NULL);

    // Main loop to wait for signals
    while (dungeon->running) {
        pause(); // Pause and wait for signals indefinitely
    }

    // Clean up resources after the main loop exits
    munmap(dungeon, sizeof(struct Dungeon)); // Unmap the shared memory
    close(shm_fd); // Close the shared memory file descriptor
    sem_close(lever); // Close the semaphore

    return EXIT_SUCCESS;
}
