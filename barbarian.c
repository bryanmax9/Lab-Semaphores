#include "dungeon_info.h"
#include "dungeon_settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

struct Dungeon *dungeon; // Global shared Dungeon structure pointer

// Signal handler for DUNGEON_SIGNAL
void handle_signal(int sig) {
    if (sig == DUNGEON_SIGNAL) {
        // Copy the enemy's health into the Barbarian's attack field
        dungeon->barbarian.attack = dungeon->enemy.health;
        // Log for understanding (can be removed or commented out in production)
        printf("Barbarian attack set to: %d\n", dungeon->barbarian.attack);
    }
}

// Setup shared memory access
void setup_shared_memory() {
    int shm_fd = shm_open(dungeon_shm_name, O_RDWR, 0660);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }
    dungeon = mmap(NULL, sizeof(struct Dungeon), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (dungeon == MAP_FAILED) {
        perror("mmap failed");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }
}

int main() {
    setup_shared_memory();

    // Setup signal handling
    struct sigaction sa = {0};
    sa.sa_handler = &handle_signal;
    if (sigaction(DUNGEON_SIGNAL, &sa, NULL) == -1) {
        perror("sigaction failed");
        exit(EXIT_FAILURE);
    }

    // Wait for signal
    while (dungeon->running) {
        pause(); // Wait for signal indefinitely
    }

    // Cleanup before exiting
    if (munmap(dungeon, sizeof(struct Dungeon)) == -1) {
        perror("munmap failed");
        exit(EXIT_FAILURE);
    }
    
    return 0;
}
