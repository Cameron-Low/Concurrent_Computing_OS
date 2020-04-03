#include "hilevel.h"

// Process table
plist_t* ptable;

// Multi-level feedback queue
plist_t* multiq[MAX_PRIORITY + 1];

// Currently running process
pcb_t* running = NULL;

// Global file table
fd_entry_t fdtable[MAX_FILES];
int next_fd = 4;

// Inode table
inode_t root;

// Add pcb to correct priority ready queue
void make_ready(pcb_t* pcb) {
    // Assign the process a time slice (2^(MAX_PRIORITY - priority_level))
    int temp = 1;
    for (int i = 0; i < (MAX_PRIORITY - pcb->priority); i++) {
        temp *= 2;
    }
    pcb->timeslice = temp;
    // Push to the ready queue
    pcb->state = READY;
    push_list(multiq[pcb->priority], pcb);
}

// Load a process into the process table
void load_PCB(pcb_t* pcb) {
    push_list(ptable, pcb);
    make_ready(pcb);
}

// Perform a context switch
void dispatch(ctx_t* ctx, pcb_t* new) {
    // If there was a program running, save it's context
    if (running != NULL) {
        memcpy(&running->ctx, ctx, sizeof(ctx_t));
    }

    // Update the running process
    memcpy(ctx, &new->ctx, sizeof(ctx_t));
    running = new;
    running->state = RUNNING;
}

// Pick the next process to execute from the ready queue
void schedule(ctx_t* ctx) {
    if (running != NULL) {
        // If the process used all its time slice then lower priority, otherwise raise it
        if (running->timeslice == 0) {
            running->priority = running->priority - 1 < 0 ? 0 : running->priority - 1;
        } else {
            running->priority = running->priority + 1 > MAX_PRIORITY ? MAX_PRIORITY : running->priority + 1;
        }
        // Add it to the correct ready queue
        make_ready(running);
    }

    // Schedule the new process
    for (int i = MAX_PRIORITY; i >= 0; i--) {
        if (!is_empty(multiq[i])) {
            dispatch(ctx, pop_list(multiq[i]));
            return;
        }    
    }
}

// Hi-level code for handling RST interrupts
void hilevel_handler_rst(ctx_t* ctx) {
    // Setup timer to cause an interupt every 1 second
    TIMER0->Timer1Load = 0x1000;
    TIMER0->Timer1Ctrl = 0xE2;
    
    // Setup GIC so that timer interrupts are allowed through to the processor via IRQ
    GICC0->PMR = 0xF0;
    GICC0->CTLR = 0x1;
    GICD0->ISENABLER1 = 0x10;
    GICD0->CTLR = 0x1;

    // Initialise process table
    if (ptable != NULL) {
        free_list(ptable);
    }
    ptable = create_list();

    // Load the root inode into memory
    read_inode(get_inode(), &root);

    // Initialise global file descriptor table
    for (int i = 0; i < 4; i++) {
        fdtable[i] = (fd_entry_t) {i, -1};
    }
    
    // Initialise the ready queue
    for (int i = 0; i < MAX_PRIORITY + 1; i++) {
        if (multiq[i] != NULL) {
            free_list(multiq[i]);
        }
        multiq[i] = create_list();
    }

    // Load the console program into the process table and run it
    init_stacks();
    pcb_t* cons = create_PCB("console", (uint32_t) &main_console, NULL);
    cons->fdtable[1] = 3;
    load_PCB(cons);

    schedule(ctx);
    
    // Remove IRQ interrupt mask
    int_enable_irq();
}

// Hi-level code for handling IRQ interrupts
void hilevel_handler_irq(ctx_t* ctx) {
    // Get the id of the interrupting device
    uint32_t id = GICC0->IAR;

    switch(id) {
        case GIC_SOURCE_TIMER0: {
            // Call scheduler if the time slice has been used
            running->timeslice--;
            if (running->timeslice == 0) {
                schedule(ctx);
            }

            // Clear the interrupt from the timer
            TIMER0->Timer1IntClr = 0x1;                        
        }
    }

    // Interrupt handled
    GICC0->EOIR = id;
}

// Hi-level code for handling SVC interrupts
void hilevel_handler_svc(ctx_t* ctx, uint32_t id) {
    switch(id) {
        // Handle yield system call
        case 0x00: {
            // Call the scheduler
            schedule(ctx);
            break;           
        }
        // Handle the write system call
        case 0x01: {
            // Get the file descriptor (not used), string pointer and length of string.
            uint32_t usr_fd = ctx->gpr[0];
            char* str = (char*) ctx->gpr[1];
            uint32_t len = ctx->gpr[2];
            
            int fd = running->fdtable[usr_fd];

            if (fd == 3) { 
                // Push string byte-by-byte, to UART0 (connected to terminal window) so that a character is printed
                for (int i = 0; i < len; i++) {
                    PL011_putc(UART1, *str++, true);
                }
            } else if (fd == 1 || fd == 2) {
                // Push string byte-by-byte, to UART1 (connected to console window) so that a character is printed
                for (int i = 0; i < len; i++) {
                    PL011_putc(UART0, *str++, true);
                } 
            } else {
                inode_t inode;
                read_inode(fdtable[fd].inode_num, &inode);
                uint8_t block[64] = {0};
                int full_blocks = len / 64;
                int rem = len % 64;
                for (int i = 0; i < full_blocks; i++) {
                    memcpy(block, str, 64);
                    if (inode.directptrs[i] != 0) {
                        free_data_block(inode.directptrs[i]);
                    }
                    inode.directptrs[i] = get_data();
                    write_inode(fdtable[fd].inode_num, &inode);
                    disk_wr(inode.directptrs[i], block);
                }
                if (rem != 0) {
                    uint8_t block[64] = {0};
                    memcpy(block, str, rem);
                    if (inode.directptrs[full_blocks] != 0) {
                        free_data_block(inode.directptrs[full_blocks]);
                    }
                    inode.directptrs[full_blocks] = get_data();
                    write_inode(fdtable[fd].inode_num, &inode);
                    disk_wr(inode.directptrs[full_blocks], block);
                }
                
            }
            
            // Return the number of bytes written to file
            ctx->gpr[0] = len;
            break;
        }
         // Handle the read system call
        case 0x02: {
            // Get the file descriptor (not used), string pointer and length of string.
            uint32_t fd = ctx->gpr[0];
            char* str = (char*) ctx->gpr[1];
            uint32_t len = ctx->gpr[2];

            // Get string byte-by-byte, from UART1 (connected to console) so that a character is received
            for (int i = 0; i < len; i++) {
                *str = PL011_getc(UART1, true);
                str++;
            }
            
            // Return the number of bytes written to file
            ctx->gpr[0] = len;
            break;
        }
        // Handle the fork system call
        case 0x03: {
            // Create the new child process as an exact duplicate of its parent
            pcb_t* child = create_PCB(running->name, ctx->pc, running);
            memcpy(&child->ctx, ctx, sizeof(ctx_t));

            // Give the child it's own stack, copied from the parent.
            uint32_t stack_offset = running->ptos - ctx->sp;
            child->ctx.sp -= stack_offset;
            memcpy((uint32_t*) child->ctx.sp, (uint32_t*) ctx->sp, stack_offset);

            // Add it to the process table
            load_PCB(child);
            
            // Set up return values for child and parent
            child->ctx.gpr[0] = 0;
            ctx->gpr[0] = child->pid;
            break;
        }
        // Handle the exit system call
        case 0x04: {
            // Get the exit status
            uint32_t status = ctx->gpr[0];

            // Remove the PCB for this process from the process table
            destroy_PCB(delete_list(ptable, running->pid));
            
            // Schedule a new process
            running = NULL;
            schedule(ctx);
            break;
        }
        // Handle the exec system call
        case 0x05: {
            // Overload the process with the new program
            ctx->pc = ctx->gpr[0];
            // Reset the stack pointer
            ctx->sp = running->ptos;
            break;
        }
        // Handle the kill system call
        case 0x06: {
            // Get the pid and signal
            uint32_t pid = ctx->gpr[0];
            uint32_t signal = ctx->gpr[1];

            // Send the signal to the process
            switch(signal) {
                // Handle the SIG_TERM and SIG_QUIT signals
                case 0x00:
                case 0x01: {
                    if (pid == 0) {
                        pnode_t* cur = ptable->head;
                        pnode_t* next = cur->next;
                        while (cur != NULL) {
                            int pid = cur->data->pid;
                            cur = cur->next;
                            if (pid != 0) {
                                // Remove the PCB for this process from the process table
                                pcb_t* temp = delete_list(ptable, pid);
                                // Remove the PCB from the correct ready queue
                                delete_list(multiq[temp->priority], pid);
                                destroy_PCB(temp);    
                            }
                        }
                    } else {
                            // Remove the PCB for this process from the process table
                            pcb_t* temp = delete_list(ptable, pid);
                            // Remove the PCB from the correct ready queue
                            delete_list(multiq[temp->priority], pid);
                            destroy_PCB(temp);
                    }
                    ctx->gpr[0] = 0;
                }
            }
            break;
        }
        // Handle the nice system call
        case 0x07: {
            // Get the pid and priority
            uint32_t pid = ctx->gpr[0];
            uint32_t priority = ctx->gpr[1];
            
            // Find the pcb in the process table and change the priority
            pcb_t* pcb = search_list(ptable, pid);
            delete_list(multiq[pcb->priority], pid);
            pcb->priority = priority;
            make_ready(pcb);
            break; 
        }
        // Handle the sem_init system call
        case 0x08: {
            uint32_t* sem = malloc(sizeof(uint32_t));
            *sem = ctx->gpr[0];
            ctx->gpr[0] = (uint32_t) sem;
            break;           
        }
        // Handle the sem_close system call
        case 0x09: {
            free((uint32_t*) ctx->gpr[0]);
            break;
        }
        // Handle the list processes call
        case 0x10: {
            char** proc_names = malloc(sizeof(char*) * num_procs);
            int* proc_ids = malloc(sizeof(int*) * num_procs);
            pnode_t* cur = ptable->head;
            for (int i = 0; i < num_procs; i++) {
                proc_names[i] = cur->data->name;
                proc_ids[i] = cur->data->pid;
                cur = cur->next;
            }
            ctx->gpr[0] = (uint32_t) proc_names;
            ctx->gpr[1] = (uint32_t) proc_ids;
            ctx->gpr[2] = num_procs;
            break; 
        }
        // Handle the open system call
        case 0x11: {
            char* path = (char*) ctx->gpr[0];
            
            inode_t inode;
            int inode_num = 0;

            // Traverse the directories in the file path
            char* file = "/";
            char* next_file = strtok(path, "/");
            while (file != NULL) {
                read_inode(inode_num, &inode);

                // Search for name
                if (inode.type == DIRECTORY) {
                    if (next_file == NULL) {
                        // Cannot 
                    }

                    dir_entry_t entry;
                    // Look for name in entries
                    for (int i = 0; i < 12; i++) {
                        // If we reach an empty location then we haven't found the file, so create it
                        if (inode.directptrs[i] == 0) {
                            // Create a new directory entry
                            entry.inode_num = get_inode();
                            entry.type = DATA;
                            strcpy(entry.name, next_file);
                            
                            // Write this entry into the directory inode
                            inode.directptrs[i] = get_data();
                            write_dir_entry(inode.directptrs[i], &entry);
                            write_inode(inode_num, &inode);
                            
                            // Create a new inode for this file
                            inode_t new_inode = {0};
                            new_inode.type = DATA;
                            write_inode(entry.inode_num, &new_inode);

                            // Add the new file to the open file table
                            fd_entry_t new = (fd_entry_t) {next_fd++, entry.inode_num};
                            fdtable[new.fd] = new;
                            running->fdtable[running->next_fd] = new.fd;
                            ctx->gpr[0] = running->next_fd;
                            return;
                        } else {
                            read_dir_entry(inode.directptrs[i], &entry);
                            // Check if we can see the entry
                            if (strcmp(next_file, entry.name) == 0) {
                                inode_num = entry.inode_num;
                                break;
                            }
                        }
                    }
                    file = next_file;
                    next_file = strtok(NULL, "/");
                } else if (inode.type == DATA) {
                    if (next_file != NULL) {
                        // Error: data treated as directory
                    }
                    // Check if file already open
                    fd_entry_t new;
                    for (int i = 0; i < MAX_FILES; i++) {
                        // Check if the file is in the process file table already
                        if (running->fdtable[i] == inode_num) {
                            ctx->gpr[0] = running->fdtable[i];
                            return;
                        }
                        // Check if the file is in the global file table already
                        if (fdtable[i].inode_num == inode_num) {
                            running->fdtable[running->next_fd] = fdtable[i].fd;
                            ctx->gpr[0] = running->next_fd++;
                            return;
                        }
                    }
                    // Otherwise create new open file descriptor
                    new = (fd_entry_t) {next_fd++, inode_num};
                    fdtable[new.fd] = new;
                    running->fdtable[running->next_fd] = new.fd;
                    ctx->gpr[0] = running->next_fd++;
                    return;
                } else {
                    // Error: bad file path
                }
            }
        }
    }
}


