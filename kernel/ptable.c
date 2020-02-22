#include "ptable.h"

// Store the value of the next pid
int nextId = 0;

// Create a new PCB for a process
pcb_t createPCB(void* entryPoint, uint32_t ptos) {
    pcb_t pcb;

    // Assign the process an id
    pcb.pid = nextId++;

    // Update process state
    pcb.pstate = CREATED;
    
    // Give the process an initial priority of 0
    pcb.priority = 0;
    pcb.timeslice = 0;

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

// Create a process queue
pqueue_t* createQ() {
    pqueue_t* q = malloc(sizeof(pqueue_t));
    q->head = NULL;
    q->tail = NULL;
    return q;
}

// Free a process queue
void freeQ(pqueue_t* q) {
    if (q->head != NULL) {
        pqnode_t* prev = q->head;
        pqnode_t* cur = prev->next;
        while (cur->next != NULL) {
            free(prev);
            prev = cur;
            cur = cur->next;
        }
        free(cur);
    }
    free(q);
}

// Add pcb to process queue
void addQ(pqueue_t* q, pcb_t* pcb, pstate_t state) {
    pcb->pstate = state;
    pqnode_t* node = malloc(sizeof(pqnode_t));
    node->data = pcb;
    node->next = NULL;
    if (q->head == NULL) {
        q->head = node;
        q->tail = node;
    } else {
        q->tail->next = node;
        q->tail = node;
    }
}

// Remove the head of the queue
pcb_t* removeQ(pqueue_t* q, pstate_t state) {
    if (q->head == NULL) {
        return NULL;
    } else {
        pqnode_t* node = q->head;
        pcb_t* pcb = node->data;
        pcb->pstate = state;
        q->head = node->next;
        free(node);
        return pcb;
    }
}
