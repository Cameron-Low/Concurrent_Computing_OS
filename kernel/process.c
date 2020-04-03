#include "process.h"

// Stack bitmap
uint32_t stacks = 0;

// Setup bitmap for all user stacks
void init_stacks() {
    for (int i = 0; i < ((uint32_t) &max_procs) + 1; i++) {
        if (i > 31){
            return;
        }
        stacks += 1 << i;
    }
}

// Mark a stack as claimed in the bitmap
uint32_t get_stack() {
    for (int i = 0; i < 32; i++) {
        uint32_t temp = 1 << i;
        if (stacks & temp) {
            stacks ^= temp;
            return i;
        }
    }
    return -1;
}

// Unmark a stack from the bitmap
void return_stack(uint32_t num) {
    stacks |= 1 << num;
} 

// Store the value of the next pid
int next_pid = 0;

// Number of processes active
int num_procs = 0;

// Create a new PCB for a process
pcb_t* create_PCB(const char* name, uint32_t entryPoint, pcb_t* parent) {
    num_procs++;
    pcb_t* pcb = malloc(sizeof(pcb_t));

    pcb->pid = next_pid++;
    pcb->state = CREATED;
    pcb->name = malloc(sizeof(char) * strlen(name) + 1);
    memcpy(pcb->name, name, sizeof(char) * strlen(name) + 1);
    pcb->parent = parent;

    pcb->priority = MAX_PRIORITY;
    pcb->timeslice = 1;
    
    // Setup initial standard file descriptors
    pcb->fdtable[0] = 0;
    pcb->fdtable[1] = 1;
    pcb->fdtable[2] = 2;
    for (int j = 3; j < MAX_FILES; j++) {
        pcb->fdtable[j] = -1;
    }
    
    // Store the value of the next file_descriptor
    pcb->next_fd = 3;

    // Set the top of the process's stack
    pcb->stack_num = get_stack();
    if (pcb->stack_num < 0) {
        // Error......
    }

    pcb->ptos = ((uint32_t) &usr_stacks) - (((uint32_t) &proc_stack_size) * pcb->stack_num);

    // Initialise context
    pcb->ctx.cpsr = 0x50;
    pcb->ctx.pc = entryPoint;
    for (int i = 0; i < 13; i++) {
        pcb->ctx.gpr[i] = 0;
    }
    pcb->ctx.sp = pcb->ptos;
    pcb->ctx.lr = 0;
    
    return pcb;
}

// Delete a process
void destroy_PCB(pcb_t* p) {
    num_procs--;
    p->state = TERMINATED;
    return_stack(p->stack_num);
    free(p);
}

// Create a process list
plist_t* create_list() {
    plist_t* l = malloc(sizeof(plist_t));
    l->head = NULL;
    l->tail = NULL;
    return l;
}

// Free a process list
void free_list(plist_t* l) {
    if (!is_empty(l)) {
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
void push_list(plist_t* l, pcb_t* pcb) {
    pnode_t* node = malloc(sizeof(pnode_t));
    node->data = pcb;
    node->next = NULL;
    if (is_empty(l)) {
        l->head = node;
        l->tail = node;
    } else {
        l->tail->next = node;
        l->tail = node;
    }
}

// Pop the first node in the list
pcb_t* pop_list(plist_t* l) {
    if (is_empty(l)) {
        return NULL;
    } else {
        pnode_t* node = l->head;
        pcb_t* pcb = node->data;
        l->head = node->next;
        if (node->next == NULL) {
            l->tail = NULL;
        }
        free(node);
        return pcb;
    }
}

// Delete a process from a list
pcb_t* delete_list(plist_t* l, int pid) {
    // Check if the list is empty
    if (is_empty(l)) {
        return NULL;
    }
    // Find the node with our process
    pnode_t* prev = NULL;
    pnode_t* cur = l->head;
    while (cur->data->pid != pid) {
        prev = cur;
        cur = cur->next;
        if (cur == NULL) {
            return NULL;
        }
    }
    
    // Perform a clean deletion
    if (cur == l->head) {
        if (cur->next == NULL) {
            l->tail = NULL;
        }
        l->head = cur->next;
    } else if (cur == l->tail) {
        l->tail = prev;
        prev->next = NULL;
    } else {
        prev->next = cur->next;
    }

    pcb_t* pcb = cur->data;
    free(cur);
    return pcb;
}

// Find a specific process from a list
pcb_t* search_list(plist_t* l, int pid) {
    if (is_empty(l)) {
        return NULL;
    }
    // Find the node with our process
    pnode_t* prev = NULL;
    pnode_t* cur = l->head;
    while (cur->data->pid != pid) {
        prev = cur;
        cur = cur->next;
        if (cur == NULL) {
            return NULL;
        }
    }
    return cur->data;
}

// Check if a list is empty
int is_empty(plist_t* l) {
    return l->head == NULL;
}
