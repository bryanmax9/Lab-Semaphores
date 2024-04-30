// wizard.c
/*
ABOUT:
When the Wizard receives a signal (defined in dungeon_settings.h), 
the Wizard reads the Caesar Cypher placed in the Barrier's spell field. 
The Wizard then decodes the Caesar Cypher, 
using the first character as the key, 
and copies the decoded message into the Wizard's spell field. 
The Dungeon will wait an amount of time defined in dungeon_settings.h 
as SECONDS_TO_GUESS_BARRIER for the decoding process to complete. 
If the Wizard's spell field matches the decoded message after the Dungeon has finished waiting, 
then this will count as success.
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
#include <ctype.h> // tolower and isalpha function
#include <semaphore.h>

struct Dungeon *dungeon;
sem_t *my_semaphore;  // global semaphore

void signal_setup();
void signal_check();
void spell_decoder();


void signal_setup() {
    struct sigaction sa = {0};
    sa.sa_handler = &signal_check;
    sigaction(DUNGEON_SIGNAL, &sa, NULL);

    if (sigaction(DUNGEON_SIGNAL, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

void signal_check(int signal_val) {

    // if signal, the decode barrier spell
    if (signal_val == DUNGEON_SIGNAL) {
        spell_decoder();
        printf("spell decoded: %s\n", dungeon->wizard.spell);

        sleep(SECONDS_TO_GUESS_BARRIER);
    }
}

void spell_decoder() {
    sem_wait(my_semaphore);
    // ceaser cypher decode formula: (x-n) \ mod 26
    if (dungeon == NULL) {
        return;
    }

    // first character of spell shift value
    int key_character = dungeon->barrier.spell[0];
    int  shift_value = key_character % 26;

    // decode starting from second character
    int i = 0;
    while (dungeon->barrier.spell[i] != '\0') {

        char current_char = dungeon->barrier.spell[i];

        if (isalpha(current_char)) {
            // check if uppercase
            if (dungeon->barrier.spell[i] >= 'A' && dungeon->barrier.spell[i] <= 'Z') {
                dungeon->wizard.spell[i-1] = 'A' + ((dungeon->barrier.spell[i] - 'A' - shift_value + 26) % 26);
            } else if (dungeon->barrier.spell[i] >= 'a' && dungeon->barrier.spell[i] <= 'z') {
                dungeon->wizard.spell[i-1] = 'a' + ((dungeon->barrier.spell[i] - 'a' - shift_value + 26) % 26);
            }
        } else { // non alphabetical characters
            dungeon->wizard.spell[i - 1] = dungeon->barrier.spell[i];
        }

        i++;
    }

    // null terminate string at end
    dungeon->wizard.spell[i-1] = '\0';
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

    // clean up unmapped shared memory
    munmap(dungeon, sizeof(struct Dungeon));
    close(shm_fd);
    sem_close(my_semaphore);

    return EXIT_SUCCESS;
}

