// Include the headers containing definitions for the Dungeon structure, and other dependencies
#include "dungeon_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <errno.h>
#include <signal.h>

// Declare the external function RunDungeon to be used later to run the dungeon along with the characters
extern void RunDungeon(pid_t wizard, pid_t rogue, pid_t barbarian);

int main() {
    // Unlink any previous semaphore handles to start fresh
    sem_unlink(dungeon_lever_one);
    sem_unlink(dungeon_lever_two);

    // Open a shared memory segment
    int shm_fd = shm_open(dungeon_shm_name, O_CREAT | O_RDWR, 0660);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }

    // Set the size of the shared memory segment to the size of Dungeon struct
    if (ftruncate(shm_fd, sizeof(struct Dungeon)) == -1) {
        perror("ftruncate failed");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    // Map the shared memory segment for read and write access
    struct Dungeon *dungeon = mmap(NULL, sizeof(struct Dungeon), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (dungeon == MAP_FAILED) {
        perror("mmap failed");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }
    // Initialize the dungeon's running state to true
    dungeon->running = true;

    // Create and open semaphores for controlling access to the treasure room
    sem_t *one = sem_open(dungeon_lever_one, O_CREAT, 0660, 1);
    if (one == SEM_FAILED) {
        perror("sem_open for leverOne failed");
        exit(EXIT_FAILURE);
    }

    sem_t *two = sem_open(dungeon_lever_two, O_CREAT, 0660, 1);
    if (two == SEM_FAILED) {
        perror("sem_open for leverTwo failed");
        exit(EXIT_FAILURE);
    }

    // Fork to create a new process for the barbarian character
    pid_t barbarian_pid = fork();
    if (barbarian_pid == -1) {
        perror("Failed to fork barbarian");
        exit(EXIT_FAILURE);
    } else if (barbarian_pid == 0) {
        // If fork returns 0, we are in the child process, execute barbarian program
        execlp("./barbarian", "barbarian", NULL);
    }

    // Fork to create a new process for the wizard character
    pid_t wizard_pid = fork();
    if (wizard_pid == -1) {
        perror("Failed to fork wizard");
        exit(EXIT_FAILURE);
    } else if (wizard_pid == 0) {
        // If fork returns 0, we are in the child process, execute wizard program
        execlp("./wizard", "wizard", NULL);
    }

    // Fork to create a new process for the rogue character
    pid_t rogue_pid = fork();
    if (rogue_pid == -1) {
        perror("Failed to fork rogue");
        exit(EXIT_FAILURE);
    } else if (rogue_pid == 0) {
        // If fork returns 0, we are in the child process, execute rogue program
        execlp("./rogue", "rogue", NULL);
    }

    // Sleep to ensure all processes are started before running the dungeon
    sleep(1);
    // Start the dungeon game logic with all three characters
    RunDungeon(wizard_pid, rogue_pid, barbarian_pid);

    // Terminate child processes to clean up
    kill(barbarian_pid, SIGTERM);
    kill(wizard_pid, SIGTERM);
    kill(rogue_pid, SIGTERM);

    // Wait for all child processes to finish to ensure proper cleanup
    int status;
    waitpid(wizard_pid, &status, 0);
    waitpid(rogue_pid, &status, 0);
    waitpid(barbarian_pid, &status, 0);

    // Close and unlink the semaphores
    sem_close(one);
    sem_close(two);
    sem_unlink(dungeon_lever_one);
    sem_unlink(dungeon_lever_two);

    // Unmap and close the shared memory
    munmap(dungeon, sizeof(struct Dungeon));
    close(shm_fd);
    shm_unlink(dungeon_shm_name);

    // Indicate successful closure of the game
    printf("Success in closing the GAME! \n");
    
    return 0;
}
