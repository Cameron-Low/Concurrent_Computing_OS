#include "libc.h"

// How many philosophers do we want
#define PHILOSOPHERS (16)

uint32_t* forks[PHILOSOPHERS];

void philosopher(int id) {
    while(1) {
        // Think for a random amount of time
        for (volatile int i = 0; i < rand() % 100000; i++) {}

        if (id % 2 == 0) {
            sem_wait(forks[id]);
            sem_wait(forks[(id + 1) % PHILOSOPHERS]);
        } else {
            sem_wait(forks[(id + 1) % PHILOSOPHERS]);
            sem_wait(forks[id]);
        }

        // Eat for a random amount of time
        printI(id);
        print(" is eating\n");
        for (volatile int i = 0; i < rand() % 100000; i++) {}

        sem_post(forks[id]);
        sem_post(forks[(id + 1) % PHILOSOPHERS]);
    }
}

// Main function, responsible for setup of the semaphores and philosopher processes
void main_dining() {
    for (int i = 0; i < PHILOSOPHERS; i++) {
        forks[i] = sem_init(1);
    }
    for (int i = 0; i < PHILOSOPHERS; i++) {
        if (0 == fork()) {
            philosopher(i);
            break;
        }
    }
    exit(EXIT_SUCCESS);
}

