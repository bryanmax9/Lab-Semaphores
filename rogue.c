// rogue.c
/* 
ABOUT:
When the Rogue receives a signal (defined in dungeon_settings.h), 
the Rogue will attempt to guess a float value to "pick" a lock. 
The Trap struct has a char for direction, 
and a boolean for locked. This puzzle is a little unique. 
The Dungeon will wait for a total amount of time defined in 
dungeon_settings.h as SECONDS_TO_PICK, 
but will check the value of the Rogue's current pick position 
using TIME_BETWEEN_ROGUE_TICKS. Notice that these two values 
are quite different, and that is because one of them uses sleep, 
and the other usleep. I recommend that you follow a similar example.

Every X microseconds, the Dungeon will check the field pick 
in the Rogue struct in shared memory, 
and will change the direction and locked fields in Trap accordingly. 
If the Rogue's pick needs to go up, 
the trap will set direction to u. 
If the Rogue's pick needs to go down, t
he trap will set the direction to d. 
If the Rogue's pick is in the right position, 
the dungeon will set direction to -, and locked to false, 
indicating that the Rogue succeeded in picking the lock. 
If this occurs, it is counted as success. 
While this can be done through a brute force search, 
it can be very elegantly accomplished with a binary search.
*/
#include "dungeon_info.h"
#include "dungeon_settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <semaphore.h>
#include <stdbool.h>

struct Dungeon *dungeon;
sem_t* semaphore_one;
sem_t* semaphore_two;
sem_t *my_semaphore;

void signal_setup();
void signal_check();

void signal_setup() {
    struct sigaction sa = {0};
    sa.sa_handler = &signal_check;
    sigaction(SEMAPHORE_SIGNAL, &sa, NULL);
}

void signal_check(int signal_val) {


    if(signal_val == DUNGEON_SIGNAL) {

        sem_wait(my_semaphore);

        float min_pick = 0.0;
        float max_pick = MAX_PICK_ANGLE;
        float middle_pick = ( max_pick + min_pick ) / 2; 

        unsigned int total_time = SECONDS_TO_PICK  * 1000000;
        unsigned int elapsed_time = 0;


        // binary search
        while ((max_pick - min_pick > LOCK_THRESHOLD) && (elapsed_time < total_time)) {
            middle_pick = ( max_pick + min_pick ) / 2;
            dungeon->rogue.pick = middle_pick;
            printf("Middle pick updated to: %f\n", middle_pick);

            // current pick position
            usleep(TIME_BETWEEN_ROGUE_TICKS);

            // update time
            elapsed_time += TIME_BETWEEN_ROGUE_TICKS;

            // adjust search
            if (dungeon->trap.direction == 'u') {
                // we need to go up
                min_pick = middle_pick;
            } else if (dungeon->trap.direction == 'd') {
                // we need to go down
                max_pick = middle_pick;
            } else if (dungeon->trap.direction == '-') {
                // pick guessed correctly
                printf("rogue has successfully picked the lock!\n");
                break;
            }
        }

        // failed statement
        if (elapsed_time >= total_time) {
            printf("Failed to pick the lock within the allowed time.\n");
        }

        // ensure resource or critical condition is available
        // sem_wait(semaphore_one);
        // sem_wait(semaphore_two);

        sleep(TIME_TREASURE_AVAILABLE);

        // release resource 
        // sem_post(semaphore_one);
        // sem_post(semaphore_two);
    }
    
    sem_post(my_semaphore);

}

int main() {
    // create/open a shared memory object
    // name of shared obj: dungeon_shm_name
    // O_RDWR: flag that allows  the shared memory object to be opened for both reading and writing
    // 0666: allow read and write access for the user, group, and others
    int shm_fd = shm_open(dungeon_shm_name, O_RDWR, 0666);

    // check if failure to open
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // setting the size of the shared memory segment
    if (ftruncate(shm_fd, sizeof(struct Dungeon)) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    // mapped memory is readable, writeable
    dungeon = mmap(NULL, sizeof(struct Dungeon), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (dungeon == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }
    // opening semaphore
    my_semaphore = sem_open("/Lever_one", 0);

    if (my_semaphore == SEM_FAILED) {
        perror("Failed to open semaphore /Lever_one");
        exit(EXIT_FAILURE);
    }

    // signal handler
    signal_setup();

    // waiting until signal 
    while (dungeon->running) {
        pause(); 
    }

    // clean up shared memory 
    // sem_close(semaphore_one);
    // sem_close(semaphore_two);
    munmap(dungeon, sizeof(struct Dungeon));
    close(shm_fd);
    sem_close(my_semaphore);
    return 0;
}