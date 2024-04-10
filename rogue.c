#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "dungeon_info.h"
#include "dungeon_settings.h"

struct Dungeon* dungeon;
sem_t* lever_one;
sem_t* lever_two;

// Signal handler to perform binary search and semaphore operations
void signal_handler(int sig) {
    if (sig == SEMAPHORE_SIGNAL) {
        // Binary search for lock picking
        float low = 0.0, high = MAX_PICK_ANGLE, mid;
        while (high - low > LOCK_THRESHOLD) {
            mid = (low + high) / 2;
            dungeon->rogue.pick = mid;

            // Assuming dungeon updates trap.direction based on the guess
            usleep(TIME_BETWEEN_ROGUE_TICKS); // Simulate time between checks

            if (dungeon->trap.direction == 'u') {
                low = mid;
            } else if (dungeon->trap.direction == 'd') {
                high = mid;
            } else {
                // Correct guess, trap unlocked
                break;
            }
        }

        // After unlocking, handle the treasure room door semaphore
        sem_wait(lever_one);
        sem_wait(lever_two);

        // Simulate rogue getting the treasure. This part is simplified;
        // Implement according to your game logic, e.g., copying treasure to spoils.
        sleep(TIME_TREASURE_AVAILABLE);

        sem_post(lever_one);
        sem_post(lever_two);
    }
}

int main() {
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

    // Open semaphores
    lever_one = sem_open(dungeon_lever_one, 0);
    lever_two = sem_open(dungeon_lever_two, 0);
    if (lever_one == SEM_FAILED || lever_two == SEM_FAILED) {
        perror("sem_open failed");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa = {0};
    sa.sa_handler = &signal_handler;
    sigaction(SEMAPHORE_SIGNAL, &sa, NULL);

    while (dungeon->running) {
        pause(); // Wait for signals indefinitely
    }

    // Cleanup
    sem_close(lever_one);
    sem_close(lever_two);
    munmap(dungeon, sizeof(struct Dungeon));
    close(shm_fd);

    return 0;
}
