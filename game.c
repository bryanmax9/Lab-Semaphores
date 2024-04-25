#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include "dungeon_info.h"

// Placeholder for RunDungeon declaration
// Ensure to declare RunDungeon appropriately as per your project's specifics
extern void RunDungeon(pid_t wizard, pid_t rogue, pid_t barbarian);

int main() {
    // Set up shared memory for communication between processes
    int shm_fd = shm_open(dungeon_shm_name, O_CREAT | O_RDWR, 0660);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }
    ftruncate(shm_fd, sizeof(struct Dungeon));

    struct Dungeon* dungeon = mmap(0, sizeof(struct Dungeon), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (dungeon == MAP_FAILED) {
        perror("mmap failed");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }
    dungeon->running = true; // Start the game running

    // Fork the Barbarian process
    pid_t pid_barbarian = fork();
    if (pid_barbarian == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if (pid_barbarian == 0) {
        // Child process for Barbarian
        execlp("./barbarian", "barbarian", NULL);
        perror("Failed to start barbarian process");
        exit(EXIT_FAILURE);
    }

    // Fork the Wizard process
    pid_t pid_wizard = fork();
    if (pid_wizard == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if (pid_wizard == 0) {
        // Child process for Wizard
        execlp("./wizard", "wizard", NULL);
        perror("Failed to start wizard process");
        exit(EXIT_FAILURE);
    }

    // Fork the Rogue process
    pid_t pid_rogue = fork();
    if (pid_rogue == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if (pid_rogue == 0) {
        // Child process for Rogue
        execlp("./rogue", "rogue", NULL);
        perror("Failed to start rogue process");
        exit(EXIT_FAILURE);
    }

    // Open and initialize the semaphores
    sem_t* lever_one = sem_open(dungeon_lever_one, O_CREAT, 0660, 1); // Initialize to 1
    if (lever_one == SEM_FAILED) {
        perror("Failed to open semaphore for lever one");
        exit(EXIT_FAILURE);
    }
    sem_t* lever_two = sem_open(dungeon_lever_two, O_CREAT, 0660, 1); // Initialize to 1
    if (lever_two == SEM_FAILED) {
        perror("Failed to open semaphore for lever two");
        exit(EXIT_FAILURE);
    }

    // Call RunDungeon with the PIDs of the character classes that were launched
    RunDungeon(pid_wizard, pid_rogue, pid_barbarian);
    
    // Wait for all child processes to finish
    int status;
    while (wait(&status) > 0);
    
    // Clean up shared memory and semaphores
    sem_close(lever_one);
    sem_close(lever_two);
    sem_unlink(dungeon_lever_one);
    sem_unlink(dungeon_lever_two);
    munmap(dungeon, sizeof(struct Dungeon));
    close(shm_fd);
    shm_unlink(dungeon_shm_name);
    
    return 0;
}
