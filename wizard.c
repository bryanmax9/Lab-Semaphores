#include "dungeon_info.h"
#include "dungeon_settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>

// Global shared memory pointer to Dungeon
struct Dungeon *dungeon;

// Prototype for the signal handler function
void signal_handler(int sig);

// Prototype for the decoding function
char decode_char(char ch, int shift);

// Function to setup signal handling
void setup_signal_handling() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    if (sigaction(DUNGEON_SIGNAL, &sa, NULL) < 0) {
        perror("sigaction failed");
        exit(EXIT_FAILURE);
    }
}

// Main function setting up shared memory and signal handling
int main() {
    int shm_fd = shm_open(dungeon_shm_name, O_RDWR, 0660);
    if (shm_fd < 0) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }

    dungeon = mmap(NULL, sizeof(struct Dungeon), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (dungeon == MAP_FAILED) {
        perror("mmap failed");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    setup_signal_handling();

    while (dungeon->running) {
        printf("Waiting for signals...\n"); // Print waiting message
        pause(); // Wait for signals
    }

    printf("Game ended. Cleaning up...\n"); // Print cleanup message

    if (munmap(dungeon, sizeof(struct Dungeon)) == -1) {
        perror("munmap failed");
        exit(EXIT_FAILURE);
    }
    close(shm_fd);

    return EXIT_SUCCESS;
}

// Signal handler function to decode the spell
void signal_handler(int sig) {
    if (sig == DUNGEON_SIGNAL) {
        printf("Received DUNGEON_SIGNAL.\n"); // Print signal received message
        int shiftValue = dungeon->barrier.spell[0] - 'A';
        int i;
        for (i = 0; dungeon->barrier.spell[i] != '\0' && i < SPELL_BUFFER_SIZE; i++) {
            dungeon->wizard.spell[i] = decode_char(dungeon->barrier.spell[i], shiftValue);
        }
        dungeon->wizard.spell[i] = '\0'; // Ensure null-termination

        // Simulate decoding time
        printf("Decoding the spell...\n"); // Print decoding message
        sleep(SECONDS_TO_GUESS_BARRIER);
        printf("Spell decoded.\n"); // Print decoding complete message
    }
}

// Helper function to decode a character with the Caesar cipher
char decode_char(char ch, int shift) {
    if (ch >= 'A' && ch <= 'Z') {
        return 'A' + (ch - 'A' - shift + 26) % 26;
    } else if (ch >= 'a' && ch <= 'z') {
        return 'a' + (ch - 'a' - shift + 26) % 26;
    }
    // Return the character unchanged if it's not a letter
    return ch;
}
