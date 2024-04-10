#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "dungeon_info.h"

// Placeholder for RunDungeon declaration
// Ensure to declare RunDungeon appropriately as per your project's specifics
extern void RunDungeon(pid_t wizard, pid_t rogue, pid_t barbarian);

int main() {
    // Set up shared memory for communication between processes
    int shm_fd = shm_open(dungeon_shm_name, O_CREAT | O_RDWR, 0660);
    ftruncate(shm_fd, sizeof(struct Dungeon));
    struct Dungeon* dungeon = mmap(0, sizeof(struct Dungeon), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    dungeon->running = true; // Start the game running
    
    // Fork the Barbarian process
    pid_t pid_barbarian = fork();
    if (pid_barbarian == 0) {
        // Child process for Barbarian
        execlp("./barbarian", "barbarian", NULL);
        perror("Failed to start barbarian process");
        exit(EXIT_FAILURE);
    }
    
    // Fork the Wizard process
    pid_t pid_wizard = fork();
    if (pid_wizard == 0) {
        // Child process for Wizard
        execlp("./wizard", "wizard", NULL);
        perror("Failed to start wizard process");
        exit(EXIT_FAILURE);
    }
    
    // Fork the Rogue process
    pid_t pid_rogue = fork();
    if (pid_rogue == 0) {
        // Child process for Rogue
        execlp("./rogue", "rogue", NULL);
        perror("Failed to start rogue process");
        exit(EXIT_FAILURE);
    }
    
    // TODO: Setup semaphores as needed before calling RunDungeon

    // Call RunDungeon with the PIDs of the character classes that were launched
    RunDungeon(pid_wizard, pid_rogue, pid_barbarian);
    
    // Wait for all child processes to finish
    int status;
    while (wait(&status) > 0);
    
    // Clean up shared memory
    munmap(dungeon, sizeof(struct Dungeon));
    close(shm_fd);
    shm_unlink(dungeon_shm_name);
    
    return 0;
}

