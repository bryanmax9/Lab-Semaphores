#include "dungeon_info.h"
#include "dungeon_settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include <semaphore.h>

struct Dungeon *dungeon = NULL;
sem_t *my_semaphore;

void barbarian_signal_handler(int signal_val) {
    if (signal_val == SIGUSR1) {
        sem_wait(my_semaphore); // Wait to get semaphore before processing

        printf("Received signal - Enemy Health: %d\n", dungeon->enemy.health);
        dungeon->barbarian.attack = dungeon->enemy.health;
        printf("Barbarian Attack set: %d\n", dungeon->barbarian.attack);

        sleep(SECONDS_TO_ATTACK);

        if (dungeon->barbarian.attack == dungeon->enemy.health) {
            printf("Barbarian attack successful!\n");
        } else {
            printf("Barbarian attack failed!\n");
        }

        sem_post(my_semaphore); // Release semaphore after processing
    }
}

void setup_barbarian_signals() {
    struct sigaction sa;
    sa.sa_handler = barbarian_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(DUNGEON_SIGNAL, &sa, NULL) == -1) {
        perror("sigaction failed");
        exit(EXIT_FAILURE);
    }
}

int main() {
    int shm_fd = shm_open(dungeon_shm_name, O_RDWR, 0666);
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

    my_semaphore = sem_open("/Lever_one", 0);
    if (my_semaphore == SEM_FAILED) {
        perror("Failed to open semaphore /Lever_one");
        exit(EXIT_FAILURE);
    }

    setup_barbarian_signals();

    while (dungeon->running) {
        pause(); // Wait for signals
    }

    // Cleanup code omitted for brevity
    return 0;
}
