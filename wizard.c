// Include necessary headers for functionality like I/O, signals, shared memory, semaphore control, etc.
#include <ctype.h>
#include <semaphore.h>
#include "dungeon_info.h"
#include "dungeon_settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <fcntl.h>  // For opening shared memory
#include <sys/stat.h>  // For mode constants

struct Dungeon *dungeon; // Global pointer to shared dungeon data
sem_t *lever; // Semaphore for handling lever actions

// Function to decode spells and handle lever based on signal
void wizard_spell_signal_handler(int signal, struct Dungeon *dungeon) {
    if (signal == DUNGEON_SIGNAL) {
        sem_wait(lever); // Wait to acquire the lever semaphore before proceeding
        const char* encoded = dungeon->barrier.spell; // Access the encoded spell from the barrier
        int key = encoded[0] % 26; // Use the first character as the key for decoding
        char decoded[SPELL_BUFFER_SIZE] = {0}; // Buffer to store the decoded message

        printf("Encoded spell: %s\n", encoded);
        printf("Decoding key: %d\n", key);

        // Decode the message, skipping the first character which is the key
        for (int i = 1; encoded[i] != '\0'; i++) {
            if (isalpha(encoded[i])) {
                char base = isupper(encoded[i]) ? 'A' : 'a'; // Determine the base (A or a) depending on case
                int offset = encoded[i] - base; // Calculate the offset from the base
                decoded[i - 1] = (offset - key + 26) % 26 + base; // Decode and wrap around within the alphabet
            } else {
                decoded[i - 1] = encoded[i]; // Non-alphabet characters are copied directly
            }
        }

        strncpy(dungeon->wizard.spell, decoded, SPELL_BUFFER_SIZE); // Copy the decoded message to wizard's spell field

        printf("Decoded spell: %s\n", decoded);

        if (strncmp(decoded, dungeon->wizard.spell, SPELL_BUFFER_SIZE) == 0) {
            printf("Decoding successful!\n");
        } else {
            printf("Decoding failed.\n");
        }
        sleep(SECONDS_TO_GUESS_BARRIER); // Simulate time to guess the barrier
        sem_post(lever); // Release the lever after processing
    } 
    
    else if (signal == SEMAPHORE_SIGNAL) {
        sem_wait(lever); // Handle semaphore signal for treasure collection
        printf("Wizard is holding the lever.\n");
        while (strlen(dungeon->spoils) < 4); // Wait until all treasures are collected
        sleep(TIME_TREASURE_AVAILABLE);  // Simulate holding the lever for a set duration
        sem_post(lever); // Release the lever once done
        printf("Wizard has released the lever.\n");
    }
}

// Setup signal handling for decoding and lever holding
void Actual_Wizard_Handler(int signal, siginfo_t *info, void *ucontext) {
    wizard_spell_signal_handler(signal, dungeon);
}

int main() {
    // Open shared memory
    int shm_fd = shm_open(dungeon_shm_name, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Failed to open shared memory");
        exit(EXIT_FAILURE);
    }

    // Map the shared memory
    dungeon = mmap(NULL, sizeof(struct Dungeon), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (dungeon == MAP_FAILED) {
        perror("Failed to mmap shared memory");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    // Open semaphore for lever handling
    lever = sem_open(dungeon_lever_two, O_CREAT, 0666, 0);
    if (lever == SEM_FAILED) {
        perror("Failed to open semaphore for lever in wizard");
        exit(EXIT_FAILURE);
    }

    // Configure and apply signal actions
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = Actual_Wizard_Handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    // Register signal handlers
    if (sigaction(DUNGEON_SIGNAL, &sa, NULL) != 0 || sigaction(SEMAPHORE_SIGNAL, &sa, NULL) != 0) {
        perror("Sigaction failed");
        exit(EXIT_FAILURE);
    }

    printf("Wizard process started, listening for signals.\n");
    while (dungeon->running) {
        pause();  // Pause and wait for signals indefinitely
    }

    return 0;
}
