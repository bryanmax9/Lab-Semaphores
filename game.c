#include "dungeon_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <semaphore.h>

volatile sig_atomic_t signal_flag = 0;

extern void RunDungeon(pid_t wizard, pid_t rogue, pid_t barbarian);
pid_t fork_and_exec(const char *path);

void game_signal_handler(int sig) {
     if (sig == DUNGEON_SIGNAL) {
        signal_flag = 1;
     } else if (sig == SEMAPHORE_SIGNAL) {
        signal_flag = 2;
     }
}

void setup_game_signal_handling() {
    struct sigaction sa;
    sa.sa_handler = game_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(DUNGEON_SIGNAL, &sa, NULL) == -1) {
        perror("sigaction failed");
        exit(EXIT_FAILURE);
    }

     if (sigaction(SEMAPHORE_SIGNAL, &sa, NULL) == -1) {
        perror("SEMAPHORE_SIGNAL failed");
        exit(EXIT_FAILURE);
    }
}


int main() {
    setup_game_signal_handling();

    int shm_fd = shm_open(dungeon_shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, sizeof(struct Dungeon)) == -1) {
        perror("ftruncate");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    struct Dungeon *dungeon = mmap(0, sizeof(struct Dungeon), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (dungeon == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    dungeon->running = true;

    sem_t *sem1 = sem_open("/Lever_one", O_CREAT, 0666, 1);
    if (sem1 == SEM_FAILED) {
        perror("sem_open /Lever_one");
        exit(EXIT_FAILURE);
    }

    sem_t *sem2 = sem_open("/Lever_two", O_CREAT, 0666, 1);
    if (sem2 == SEM_FAILED) {
        perror("sem_open /Lever_two");
        exit(EXIT_FAILURE);
    }

    // Initialize characters
    pid_t barbarian_pid = fork_and_exec("./barbarian");
    pid_t wizard_pid = fork_and_exec("./wizard");
    pid_t rogue_pid = fork_and_exec("./rogue");

    RunDungeon(wizard_pid, rogue_pid, barbarian_pid);

    while (dungeon->running) {
        pause();

        if (signal_flag == 1) {
            if (kill(barbarian_pid, SIGUSR1) == -1) {
                perror("Failed to signal barbarian");
            }
            signal_flag = 0;

            sleep(1);

            if (kill(wizard_pid, DUNGEON_SIGNAL) == -1) {
                perror("Failed to signal wizard");
            }
            signal_flag = 0;

            sleep(1);

            if (kill(rogue_pid, DUNGEON_SIGNAL) == -1) {
                perror("Failed to signal rogue");
            }
            signal_flag = 0;


        }

    }
    // wait for all children
    int process_status;
    while (wait(&process_status) > 0);

    // cleanup
    sem_close(sem1);
    sem_close(sem2);
    sem_unlink("/Lever_one");
    sem_unlink("/Lever_two");

    munmap(dungeon, sizeof(struct Dungeon));
    close(shm_fd);
    shm_unlink(dungeon_shm_name);

    return 0;
}

pid_t fork_and_exec(const char *path) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        execlp(path, path, NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    }
    return pid;
}
