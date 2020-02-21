#include "ptable.h"

// Store the value of the next pid.
int nextId = 0;

pcb_t createPCB(void* entryPoint, uint32_t ptos) {
    pcb_t pcb;

    // Assign the process an id
    pcb.pid = nextId++;

    // Update process state
    pcb.pstate = CREATED;

    // Set the top of the process's stack
    pcb.ptos = (uint32_t) &ptos;

    // Initialise the context

    // Set cpsr to usr mode
    pcb.ctx.cpsr = 0x50;

    // Set pc to beginning of process
    pcb.ctx.pc = (uint32_t) entryPoint;

    // Initialise general purpose registers
    for (int i = 0; i < 13; i++) {
        pcb.ctx.gpr[i] = 0;
    }

    // Initialise the stack pointer to the top of the allocated stack for this process
    pcb.ctx.sp = pcb.ptos;
    
    // Initialise the link register
    pcb.ctx.lr = 0;

    return pcb;
}

