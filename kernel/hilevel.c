#include "hilevel.h"

// Process table
plist_t* ptable;

// Multi-level feedback queue
plist_t* multiq[MAX_PRIORITY + 1];

// Currently running process
pcb_t* running = NULL;

// Global file table
fcb_t file_table[MAX_FILES];
int next_fd = 4;

void get_next_global_fd() {
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].fd == -1) {
            next_fd = i;
            return;
        }
    }
}

// Push string to a given UART
void print_UART(PL011_t* UART, char* str, int len) {
    for (int i = 0; i < len; i++) {
        PL011_putc(UART, *str++, true);
    }
    //PL011_putc(UART, '\n', true);
}

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

    // Initialise the standard file descriptors: STDIN/STDOUT/STDERR/CONOUT
    file_table[0] = (fcb_t) {0, -1, READ};
    file_table[1] = (fcb_t) {1, -1, WRITE}; 
    file_table[2] = (fcb_t) {2, -1, WRITE}; 
    file_table[3] = (fcb_t) {3, -1, WRITE}; 
    for (int i = 4; i < MAX_FILES; i++) {
        file_table[i] = (fcb_t) {-1, -1, WRITE};
    }
    
    // Initialise the ready queue
    for (int j = 0; j < MAX_PRIORITY + 1; j++) {
        if (multiq[j] != NULL) {
            free_list(multiq[j]);
        }
        multiq[j] = create_list();
    }

    // Initialise process table
    if (ptable != NULL) {
        free_list(ptable);
    }
    ptable = create_list();

    // Set up the stacks for the user process
    init_stacks();
    
    // Create the console startup process and change its stdout to conout
    pcb_t* cons = create_PCB("console", (uint32_t) &main_console, NULL);
    cons->fdtable[1] = 3;
    load_PCB(cons);

    // Schedule the console program
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
            // Get the file descriptor, string pointer and length of string.
            uint32_t usr_fd = ctx->gpr[0];
            char* str = (char*) ctx->gpr[1];
            uint32_t len = ctx->gpr[2];
            
            // Calculate the correct file descriptor from the process file table
            int fd = running->fdtable[usr_fd];

            // Print to correct screen for STDOUT/STDERR/CONOUT
            if (fd == 1 || fd == 2) { 
                print_UART(UART0, str, len);
            } else if (fd == 3) {
                print_UART(UART1, str, len);
            } else {
                // Get the corresponding inode from the file descriptor
                inode_t inode;
                int inode_num = file_table[fd].inode_num;
                read_inode_block(inode_num, &inode);

                // Calculate the number of data blocks required for the data
                uint8_t block[BLOCK_LENGTH] = {0};
                int full = len / BLOCK_LENGTH;
                int rem =len % BLOCK_LENGTH;

                // Check if the data is too big for the file
                if (full > 11 || (full == 12 && rem != 0)) {
                    print_UART(UART0, "DATA TOO LARGE\n", 15);
                    ctx->gpr[0] = 0;
                    return;
                }

                // Write the full blocks of data first
                for (int i = 0; i < full; i++) {
                    memcpy(block, str, BLOCK_LENGTH);
                    str += BLOCK_LENGTH;
                    add_inode_data(&inode, inode_num, i, block);
                }
                // Then write the non-full block (if it exists)
                if (rem != 0) {
                    uint8_t block[BLOCK_LENGTH] = {0};
                    memcpy(block, str, rem);
                    add_inode_data(&inode, inode_num, full, block);
                }
            }
            
            // Return the number of bytes written to file
            ctx->gpr[0] = len;
            break;
        }
         // Handle the read system call
        case 0x02: {
            // Get the file descriptor, string pointer and length of string.
            uint32_t usr_fd = ctx->gpr[0];
            char* str = (char*) ctx->gpr[1];
            uint32_t len = ctx->gpr[2];

            // Calculate the correct file descriptor from the process file table
            int fd = running->fdtable[usr_fd];

            // Handle STDIN
            if (fd == 0) {
                for (int i = 0; i < len; i++) {
                    str[i] = PL011_getc(UART1, true);
                }
            } else {
                // Get the corresponding inode from the file descriptor
                inode_t inode;
                int inode_num = file_table[fd].inode_num;
                read_inode_block(inode_num, &inode);
                
                // Calculate the number of data blocks required for the data
                uint8_t block[BLOCK_LENGTH] = {0};
                int full = len / BLOCK_LENGTH;
                int rem = len % BLOCK_LENGTH;

                // Check if the data is too big for the file
                if (full > 11 || (full == 12 && rem != 0)) {
                    print_UART(UART0, "DATA TOO LARGE\n", 15);
                    ctx->gpr[0] = 0;
                    return;
                }

                // Read the full blocks of data first
                for (int i = 0; i < full; i++) {
                    read_data_block(inode.directptrs[i], block);
                    memcpy(&str[i * BLOCK_LENGTH], block, BLOCK_LENGTH);
                }
                // Then read the non-full block (if it exists)
                if (rem != 0) {
                    uint8_t block[BLOCK_LENGTH] = {0};
                    read_data_block(inode.directptrs[full], block);
                    memcpy(str, block, rem);
                }
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
                        // Terminate all processes other than the console
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
            
            // Check if we are trying to increase our own priority
            if (running->pid == pid) {
                running->priority = priority;
                break;
            }

            // Find the pcb in the process table and change the priority
            pcb_t* pcb = search_list(ptable, pid);
            delete_list(multiq[pcb->priority], pid);
            pcb->priority = priority;
            make_ready(pcb);
            break; 
        }
        // Handle the sem_init system call
        case 0x08: {
            // Create a semaphore
            uint32_t* sem = malloc(sizeof(uint32_t));
            // Initialise the value
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
            // Go through the process table and collect the ids and names
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
            // Calculate the absolute path to the new directory
            char* rel_path = (char*) ctx->gpr[0];
            char* abs_path = malloc(sizeof(char) * MAX_PATH);
            strcpy(abs_path, running->cwd);
            calculate_path(abs_path, rel_path);

            // Start our search from '/' (root directory)
            inode_t inode;
            int inode_num = 0;

            char* file = "/";
            char* next_file = strtok(abs_path, "/");
            char* next_next_file = strtok(NULL, "/");
            while (file != NULL) {
                read_inode_block(inode_num, &inode);

                if (inode.type == DIRECTORY) {
                    if (next_file == NULL) {
                        // Error: File path is invalid, trying to open a directory 
                        break;
                    }

                    // Look through each entry in the current directory to see if we can find the file
                    dir_entry_t entry = {0};
                    int free_entry_num = -1;
                    int found = 0;
                    for (int i = 0; i < 12; i++) {
                        // Keep track of the first free location, in case we need to add a new entry
                        if (inode.directptrs[i] == -1) {
                            free_entry_num = free_entry_num == -1 ? i : free_entry_num;
                        } else {
                            // Check to see if the current entry matches the next part of the path 
                            read_dir_entry(inode.directptrs[i], &entry);
                            if (strcmp(next_file, entry.name) == 0) {
                                inode_num = entry.inode_num;
                                found = 1;
                                break;
                            }
                        }
                    }

                    // If we didn't find the file then create it 
                    if (!found) {
                        if (next_next_file != NULL) {
                            // Error: File path is invalid, directory not found
                            break;
                        }
                        // Create a new directory entry
                        entry.inode_num = claim_inode_block();
                        entry.type = DATA;
                        strcpy(entry.name, next_file);
                        
                        // Write this entry into the directory inode
                        inode.directptrs[free_entry_num] = claim_data_block();
                        write_dir_entry(inode.directptrs[free_entry_num], &entry);
                        write_inode_block(inode_num, &inode);
                        
                        // Create a new inode for this file
                        inode_t new_inode;
                        for (int i = 0; i < 12; i++) {
                            new_inode.directptrs[i] = -1;
                        }
                        new_inode.type = DATA;
                        write_inode_block(entry.inode_num, &new_inode);

                        // Add the new file to the open file table
                        fcb_t new = (fcb_t) {next_fd, entry.inode_num, WRITE};
                        file_table[new.fd] = new;
                        running->fdtable[running->next_fd] = new.fd;
                        ctx->gpr[0] = running->next_fd;
                        get_next_fd(running);
                        get_next_global_fd();
                        break;
                    } else {
                        // Move on to the next part of the file path
                        file = next_file;
                        next_file = next_next_file;
                        next_next_file = strtok(NULL, "/");
                    }
                } else if (inode.type == DATA) {
                    if (next_file != NULL) {
                        // Error: data treated as directory
                        break;
                    }

                    // Check if file already open
                    fcb_t new;
                    for (int i = 0; i < MAX_FILES; i++) {
                        // Check if the file is in the process file table already
                        if (file_table[running->fdtable[i]].inode_num == inode_num) {
                            ctx->gpr[0] = i;
                            free(abs_path);
                            return;
                        }
                        // Check if the file is in the global file table already
                        if (file_table[i].inode_num == inode_num) {
                            running->fdtable[running->next_fd] = file_table[i].fd;
                            ctx->gpr[0] = running->next_fd;
                            get_next_fd(running);
                            free(abs_path);
                            return;
                        }
                    }
                    // Otherwise create new open file descriptor
                    new = (fcb_t) {next_fd, inode_num, WRITE};
                    file_table[new.fd] = new;
                    running->fdtable[running->next_fd] = new.fd;
                    ctx->gpr[0] = running->next_fd;
                    get_next_fd(running);
                    get_next_global_fd();
                    break;
                } else {
                    // Error: Invalid inode
                    break;
                }
            }
            free(abs_path);
            break;
        }
        // Handle the close system call
        case 0x12: {
            // Get the file descriptor for the process 
            int usr_fd = (int) ctx->gpr[0];
            running->fdtable[usr_fd] = -1;
            break;
        }
        // Handle the remove system call
        case 0x13: {
            // Calculate the absolute path to the new directory
            char* rel_path = (char*) ctx->gpr[0];
            char* abs_path = malloc(sizeof(char) * MAX_PATH);
            strcpy(abs_path, running->cwd);
            calculate_path(abs_path, rel_path);

            // Start our search from '/' (root directory)
            inode_t inode;
            inode_t prev_inode;
            int inode_num = 0;
            int prev_inode_num = 0;
            int dir_entry_num = 0;

            char* file = "/";
            char* next_file = strtok(abs_path, "/");
            while (file != NULL) {
                read_inode_block(inode_num, &inode);

                if (inode.type == DIRECTORY) {
                    if (next_file == NULL) {
                        // Error: 
                        break;
                    }

                    dir_entry_t entry = {0};
                    // Look for name in entries
                    for (int i = 0; i < 12; i++) {
                        // Ignore empty entries
                        if (inode.directptrs[i] == -1) {
                            continue;
                        }
                        // Check to see if the current entry matches the next part of the path 
                        read_dir_entry(inode.directptrs[i], &entry);
                        if (strcmp(next_file, entry.name) == 0) { 
                            dir_entry_num = i;
                            prev_inode_num = inode_num;
                            inode_num = entry.inode_num;
                            prev_inode = inode;
                            break;
                        }
                    }
                    // Move on to the next part of the file path
                    file = next_file;
                    next_file = strtok(NULL, "/");
                } else if (inode.type == DATA) {
                    if (next_file != NULL) {
                        // Error: data treated as directory
                        break;
                    }
                    // Free all the associated data blocks for this 
                    for (int i = 0; i < 12; i++) {
                        // Ignore unused pointers
                        if (inode.directptrs[i] != -1) {
                            free_data_block(inode.directptrs[i]);
                        }
                    }
                    // Free the directory entry for this file
                    free_data_block(prev_inode.directptrs[dir_entry_num]);
                    prev_inode.directptrs[dir_entry_num] = -1;
                    write_inode_block(prev_inode_num, &prev_inode);

                    // Free the inode for the file
                    free_inode_block(inode_num);

                    // Check if file already open
                    for (int i = 0; i < MAX_FILES; i++) {
                        // Check if the file is in the process file table 
                        if (file_table[running->fdtable[i]].inode_num == inode_num) {
                            running->fdtable[i] = -1;
                            file_table[running->fdtable[i]].fd = -1;
                            file_table[running->fdtable[i]].inode_num = -1;
                            break;
                        }
                        // Check if the file is in the global file table 
                        if (file_table[i].inode_num == inode_num) {
                            file_table[i].fd = -1;
                            file_table[i].inode_num = -1;
                            break;
                        }
                    }
                    break;
                } else {
                    // Error: bad file path
                    break;
                }
            }
            free(abs_path);
            break;
        }
        // Handle the mkdir system call
        case 0x14: {
            // Calculate the absolute path to the new directory
            char* rel_path = (char*) ctx->gpr[0];
            char* abs_path = malloc(sizeof(char) * MAX_PATH);
            strcpy(abs_path, running->cwd);
            calculate_path(abs_path, rel_path);

            // Start our search from '/' (root directory)
            inode_t inode;
            int inode_num = 0;
            int prev_dir_ptr = 0;

            char* file = "/";
            char* next_file = strtok(abs_path, "/");
            char* next_next_file = strtok(NULL, "/");
            while (file != NULL) {
                read_inode_block(inode_num, &inode);

                if (inode.type == DIRECTORY) {
                    dir_entry_t entry = {0};
                    // Look for name in entries
                    for (int i = 0; i < 12; i++) {
                        // If we find a space for the new directory and we are at the end of the path, create a new directory
                        if (inode.directptrs[i] == -1) {
                            if (next_next_file == NULL) {
                                // Create a new directory entry
                                dir_entry_t dir = {0};
                                dir.inode_num = claim_inode_block();
                                dir.type = DIRECTORY;
                                strcpy(dir.name, next_file);

                                // Write this entry into the directory inode
                                inode.directptrs[i] = claim_data_block();
                                write_dir_entry(inode.directptrs[i], &dir);
                                write_inode_block(inode_num, &inode);
                                
                                // Create a new inode for this directory
                                inode_t new_inode;
                                new_inode.directptrs[0] = inode.directptrs[i]; // . entry
                                new_inode.directptrs[1] = prev_dir_ptr; // .. entry
                                for (int i = 2; i < 12; i++) {
                                    new_inode.directptrs[i] = -1;
                                }
                                new_inode.type = DIRECTORY;
                                write_inode_block(dir.inode_num, &new_inode);
                                free(abs_path);
                                return;
                            }
                        }
                        // Check to see if the current entry matches the next part of the path 
                        read_dir_entry(inode.directptrs[i], &entry);
                        if (strcmp(next_file, entry.name) == 0) {
                                inode_num = entry.inode_num;
                                prev_dir_ptr = inode.directptrs[i];
                                break;
                        }
                    }
                    // Move on to the next part of the file path
                    file = next_file;
                    next_file = next_next_file;
                    next_next_file = strtok(NULL, "/");
                } else {
                    // Error:
                    // Cannot traverse through a file or create a directory in a file
                    free(abs_path);
                    return;    
                }
            }
            break;
        }
        // Handle the rmdir system call
        case 0x15: {
            // Calculate the absolute path to the new directory
            char* rel_path = (char*) ctx->gpr[0];
            char* abs_path = malloc(sizeof(char) * MAX_PATH);
            strcpy(abs_path, running->cwd);
            calculate_path(abs_path, rel_path);

            // Start our search from '/' (root directory)
            inode_t inode;
            int inode_num = 0;

            char* file = "/";
            char* next_file = strtok(abs_path, "/");
            char* next_next_file = strtok(NULL, "/");
            while (file != NULL) {
                read_inode_block(inode_num, &inode);

                if (inode.type == DIRECTORY) {
                    dir_entry_t entry = {0};
                    // Look for name in entries
                    for (int i = 0; i < 12; i++) {
                        // Ignore empty entries
                        if (inode.directptrs[i] == -1) {
                            continue;
                        }
                        // Check to see if the current entry matches the next part of the path 
                        read_dir_entry(inode.directptrs[i], &entry);
                        if (strcmp(next_file, entry.name) == 0) {
                            // If this is the directory we wish to remove, remove it
                            if (next_next_file == NULL) {
                                // Free the directory entry
                                free_data_block(inode.directptrs[i]);
                                inode.directptrs[i] = -1;
                                write_inode_block(inode_num, &inode);
                                free(abs_path);
                                return;
                            }
                            inode_num = entry.inode_num;
                            break;
                        }
                    }
                    // Move on to the next part of the file path
                    file = next_file;
                    next_file = next_next_file;
                    next_next_file = strtok(NULL, "/");
                } else if (inode.type == DATA) {
                    if (next_file != NULL) {
                        // Error: data treated as directory
                        break;
                    }
                }
            }
            free(abs_path);
            break;
        }
        // Handle the chdir system call
        case 0x16: {
            char* path = (char*) ctx->gpr[0];
            // Update the cwd
            calculate_path(running->cwd, path);
            break;
        }
        // Handle the getcwd system call
        case 0x17: {
            ctx->gpr[0] = (uint32_t) running->cwd;
            break;
        }
        // Handle the listdir system call
        case 0x18: {
            // Calculate the absolute path to the new directory
            char* rel_path = (char*) ctx->gpr[0];
            char* abs_path = malloc(sizeof(char) * MAX_PATH);
            strcpy(abs_path, running->cwd);
            calculate_path(abs_path, rel_path);

            // Start our search from '/' (root directory)
            inode_t inode;
            int inode_num = 0;

            char* file = "/";
            char* next_file = strtok(abs_path, "/");
            while (file != NULL) {
                read_inode_block(inode_num, &inode);

                if (inode.type == DIRECTORY) {
                    dir_entry_t entry = {0};
                    // Look for name in entries
                    for (int i = 0; i < 12; i++) {
                        // Ignore empty entries
                        if (inode.directptrs[i] == -1) {
                            continue;
                        }
                        // Check to see if the current entry matches the next part of the path 
                        read_dir_entry(inode.directptrs[i], &entry);
                        if (next_file == NULL) {
                            print_UART(UART1, entry.name, strlen(entry.name));
                            print_UART(UART1, "\n", 1);
                        } else if (strcmp(next_file, entry.name) == 0) {
                            inode_num = entry.inode_num;
                            break;
                        }
                    }
                    // Move on to the next part of the file path
                    file = next_file;
                    next_file = strtok(NULL, "/");
                } else {
                   break; 
                }
            }
            free(abs_path);
            break;
        }
    }
}

void calculate_path(char* initial_path, char* added_path) {
    // Tokenize the additional path
    char* file = strtok(added_path, "/");
    while (file != NULL) {
        // Check for 'goto parent'
        if (strcmp("..", file) == 0) {
            // Find the partition to parent directory
            char* ptr = strrchr(initial_path, '/');
            // Remove the current directory from the path
            // Keep the very first '/' to indicate the root directory
            if (strchr(initial_path, '/') != ptr) {
                *ptr = '\0';
            } else {
                ptr[1] = '\0';
            }
        } else {
            // Add a separator between the parent directory and its child, unless the parent is the root directory
            if (strcmp(initial_path, "/") != 0) {
                strcat(initial_path, "/");
            }
            strcat(initial_path, file);
        }
        // Move onto the next directory
        file = strtok(NULL, "/");
    }
}
