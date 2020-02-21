#ifndef __PTABLE_H
#define __PTABLE_H

// Standard definition includes
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Size of process table
#define MAX_PROCS (2)

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
typedef struct {
    int pid;
    pstate_t pstate;
    ctx_t ctx;
    uint32_t ptos;
} pcb_t;

// Process table containing MAX_PROCS PCB entries
typedef struct {
    pcb_t table[MAX_PROCS];
} ptable_t;

// Function declarations
pcb_t createPCB(void* entryPoint, uint32_t ptos);

#endif
