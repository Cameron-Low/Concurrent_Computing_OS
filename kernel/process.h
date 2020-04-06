#ifndef __PROCCESS_H
#define __PROCCESS_H

// Standard definition includes
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Useful Constants
#define MAX_FILES (10)
#define MAX_PRIORITY (2)
#define MAX_PATH (512)

// Top of section for all user process stacks
extern uint32_t usr_stacks;
extern uint32_t max_procs;
extern uint32_t proc_stack_size;

// Context for process, i.e. all the registers associated with a process
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
    char* name;
    pstate_t state;
    struct pcb_t* parent;
    ctx_t ctx;
    uint32_t stack_num;
    uint32_t ptos;
    int priority;
    int timeslice;
    int fdtable[MAX_FILES];
    int next_fd;
    char* cwd;
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

// User stack handling
void init_stacks();

// File descriptor management
void get_next_fd(pcb_t* p);

// PCB operations
pcb_t* create_PCB(const char* name, uint32_t entryPoint, pcb_t* parent);
void destroy_PCB(pcb_t* p);

// List operations
plist_t* create_list();
void free_list(plist_t* l);
void push_list(plist_t* l, pcb_t* pcb);
pcb_t* pop_list(plist_t* l);
pcb_t* delete_list(plist_t* l, int pid);
pcb_t* search_list(plist_t* l, int pid);
int is_empty(plist_t* l);

#endif
