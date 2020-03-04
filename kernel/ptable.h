#ifndef __PTABLE_H
#define __PTABLE_H

// Standard definition includes
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

// Size of process table
#define MAX_PROCS (10)
#define PRIORITY_LEVLS (3)

// Context for process
typedef struct {
    uint32_t cpsr, pc, gpr[13], sp, lr;
} ctx_t;

// States for process 
typedef enum {
    CREATED,
    READY,
    RUNNING,
    WAITING,
    TERMINATED
} pstate_t;

// Process Control Block (PCB)
typedef struct pcb_t {
    int pid;
    pstate_t pstate;
    struct pcb_t* parent;
    ctx_t ctx;
    uint32_t ptos;
    int priority;
    int timeslice;
} pcb_t;

// Process list node
typedef struct pnode_t {
    pcb_t* data;
    struct pnode_t* next;
} pnode_t;

// Process linked list
typedef struct {
    pnode_t* head;
    pnode_t* tail;
} plist_t;

// Function declarations
pcb_t* createPCB(uint32_t entryPoint, uint32_t ptos, pcb_t* parent);
plist_t* createL();
void freeL(plist_t* q);
void pushL(plist_t* q, pcb_t* pcb, pstate_t state);
pcb_t* popL(plist_t* q, pstate_t state);
void deleteL(plist_t* q, int pid, int clear_data);

#endif
