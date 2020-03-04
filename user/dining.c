#include "dining.h"
#define PHILOSOPHERS (16)

uint32_t* forks[PHILOSOPHERS];
uint32_t* waiter;

void main_dining() {
    for (int i = 0; i < PHILOSOPHERS; i++) {
        forks[i] = sem_init(1);
    }
    waiter = sem_init(1);
    for (int i = 0; i < PHILOSOPHERS; i++) {
        if (0 == fork()) {
            philosopher(i);
            break;
        }
    }
    exit(EXIT_SUCCESS);
}

void philosopher(int i) {
    int id = i;
    while(1) {
        sem_wait(waiter);
        sem_wait(forks[id]);
        sem_wait(forks[(id + 1) % PHILOSOPHERS]);
        sem_post(waiter);

        printI(id);

        sem_post(forks[id]);
        sem_post(forks[(id + 1) % PHILOSOPHERS]);
    }
}
