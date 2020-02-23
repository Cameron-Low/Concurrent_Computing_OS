#include "ptable.h"

// Store the value of the next pid
int nextId = 1;

// Create a new PCB for a process
pcb_t* createPCB(uint32_t entryPoint, uint32_t ptos) {
    pcb_t* pcb = malloc(sizeof(pcb_t));
    // Assign the process an id
    pcb->pid = nextId++;
    // Initialise the child process array
    for (int i = 0; i < MAX_PROCS; i++) {
        pcb->child_pids[i] = 0;
    }
    // Update process state
    pcb->pstate = CREATED;
    // Give the process an initial priority of 0
    pcb->priority = 0;
    pcb->timeslice = 0;
    // Set the top of the process's stack
    pcb->ptos = (uint32_t) &ptos;
    // Set cpsr to usr mode
    pcb->ctx.cpsr = 0x50;
    // Set pc to beginning of process
    pcb->ctx.pc = entryPoint;
    // Initialise general purpose registers
    for (int i = 0; i < 13; i++) {
        pcb->ctx.gpr[i] = 0;
    }
    // Initialise the stack pointer to the top of the allocated stack for this process
    pcb->ctx.sp = pcb->ptos;
    // Initialise the link register
    pcb->ctx.lr = 0;
    return pcb;
}

// Create a process list
plist_t* createL() {
    plist_t* l = malloc(sizeof(plist_t));
    l->head = NULL;
    l->tail = NULL;
    return l;
}

// Free a process list
void freeL(plist_t* l) {
    if (l->head != NULL) {
        pnode_t* prev = l->head;
        pnode_t* cur = prev->next;
        while (cur->next != NULL) {
            free(prev);
            prev = cur;
            cur = cur->next;
        }
        free(cur);
    }
    free(l);
}

// Add process to end of list
void pushL(plist_t* l, pcb_t* pcb, pstate_t state) {
    pcb->pstate = state;
    pnode_t* node = malloc(sizeof(pnode_t));
    node->data = pcb;
    node->next = NULL;
    if (l->head == NULL) {
        l->head = node;
        l->tail = node;
    } else {
        l->tail->next = node;
        l->tail = node;
    }
}

// Pop the first node in the list
pcb_t* popL(plist_t* l, pstate_t state) {
    if (l->head == NULL) {
        return NULL;
    } else {
        pnode_t* node = l->head;
        pcb_t* pcb = node->data;
        pcb->pstate = state;
        l->head = node->next;
        free(node);
        return pcb;
    }
}

// Delete a process from a list
void deleteL(plist_t* l, int pid, int clear) {
    // Check if the list is empty
    if (l->head == NULL) {
        return;
    }
    // Find the node with our process
    pnode_t* prev = NULL;
    pnode_t* cur = l->head;
    while (cur->data->pid != pid) {
        prev = cur;
        cur = cur->next;
        if (cur == NULL) {
            return;
        }
    }
    
    // Perform a clean deletion
    if (cur == l->head) {
        l->head = cur->next;
    } else if (cur == l->tail) {
        l->tail = prev;
        prev->next = NULL;
    } else {
        prev->next = cur->next;
    }

    // Free the space used by the node
    if (clear) {
        free(cur->data);
    }
    free(cur);
}
