#include "hilevel.h"

// Process table
plist_t* ptable;

// Multi-level feedback queue (0 is highest priority)
plist_t* multiq[PRIORITY_LEVLS];

// Currently running process
pcb_t* running = NULL;

// Time quantum counter
int tqc = 0;

// Add to the correct pcb to correct priority ready queue
void addRQ(pcb_t* pcb) {
    // Assign the process a time slice
    pcb->timeslice = exp2(pcb->priority);
    // Push to the ready queue
    pushL(multiq[pcb->priority], pcb, READY);
}

// Perform a context switch
void dispatch(ctx_t* ctx, pcb_t* current, pcb_t* new) {
    // If there was a program running, save it's context
    if (current != NULL) {
        memcpy(&current->ctx, ctx, sizeof(ctx_t));
    }

    // If we have a new process then perform a context switch
    if (new != NULL) {
        memcpy(ctx, &new->ctx, sizeof(ctx_t));
    }

    // Update the running process
    running = new;
}

// Pick the next process to execute from the ready queue
void schedule(ctx_t* ctx) {
    // If the process used all its time quantum then lower priority otherwise raise it
    if (tqc == running->timeslice) {
        running->priority = running->priority + 1 == PRIORITY_LEVLS ? running->priority : running->priority + 1;
        addRQ(running);
    } else {
        running->priority = running->priority - 1 == -1 ? running->priority : running->priority - 1;
    }
    
    // Schedule the new process
    for (int i = 0; i < PRIORITY_LEVLS; i++) {
        if (multiq[i]->head != NULL) {
            dispatch(ctx, running, popL(multiq[i], RUNNING));
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

    // Initialise process table with the console program
    ptable = createL();
    pushL(ptable, createPCB((uint32_t) &main_console, tos_Console), CREATED);

    // Initialise the ready queue
    for (int i = 0; i < PRIORITY_LEVLS; i++) {
        if (multiq[i] != NULL) {
            freeL(multiq[i]);
        }
        multiq[i] = createL();
    }

    // Add the PCB to the ready queue
    addRQ(ptable->head->data);

    // Dispatch 0th PCB entry by default
    dispatch(ctx, NULL, popL(multiq[ptable->head->data->priority], RUNNING));
    
    // Remove IRQ interrupt mask
    int_enable_irq();
}

// Hi-level code for handling IRQ interrupts
void hilevel_handler_irq(ctx_t* ctx) {
    // Get the id of the interrupting device
    uint32_t id = GICC0->IAR;

    switch(id) {
        case GIC_SOURCE_TIMER0: {
            //PL011_putc(UART0, 'T', 1);
            // Clear the interrupt from the timer
            TIMER0->Timer1IntClr = 0x1;                        

            // Call scheduler if the time quantum has been used
            tqc += 1;
            if (tqc == running->timeslice) {
                schedule(ctx);
                tqc = 0;
            }
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
            // Simply call the scheduler
            schedule(ctx);
            break;           
        }
        // Handle the write system call
        case 0x01: {
            // Get the file descriptor (not used), string pointer and length of string.
            int fd = (int) ctx->gpr[0];
            char* str = (char*) ctx->gpr[1];
            int len = (int) ctx->gpr[2];

            // Push string byte-by-byte, to UART0 (connected to terminal window) so that a character is printed
            for (int i = 0; i < len; i++) {
                PL011_putc(UART0, *str++, true);
            }
            
            // Return the number of bytes written to file
            ctx->gpr[0] = (uint32_t) len;
            break;
        }
         // Handle the read system call
        case 0x02: {
            // Get the file descriptor (not used), string pointer and length of string.
            int fd = (int) ctx->gpr[0];
            char* str = (char*) ctx->gpr[1];
            int len = (int) ctx->gpr[2];

            // Get string byte-by-byte, from UART1 (connected to console) so that a character is received
            for (int i = 0; i < len; i++) {
                *str = PL011_getc(UART1, true);
                str++;
            }
            
            // Return the number of bytes written to file
            ctx->gpr[0] = (uint32_t) len;
            break;
        }
        // Handle the fork system call
        case 0x03: {
            // Create the new child process as an exact duplicate of its parent
            pcb_t* child = createPCB(ctx->pc, ctx->sp);
            // Add it to the process table
            pushL(ptable, child, CREATED);
            memcpy(&child->ctx, ctx, sizeof(ctx_t));
            
            // Add child to the ready queue
            addRQ(child);
            
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
            deleteL(ptable, running->pid, 1);

            // Schedule a new process
            schedule(ctx);
            break;
        }
        // Handle the exec system call
        case 0x05: {
            // Get address for program entry point
            uint32_t addr = ctx->gpr[0];

            // Work out which process stack to assign to the program
            uint32_t tos = 0;
            if (addr == (uint32_t) &main_P3) {
                tos = (uint32_t) &tos_P3; 
            } else if (addr == (uint32_t) &main_P4) {
                tos = (uint32_t) &tos_P4; 
            } else if (addr == (uint32_t) &main_P5) {
                tos = (uint32_t) &tos_P5; 
            }

            // Overload the process with the new program
            ctx->pc = addr;
            ctx->sp = tos;
            break;
        }
        // Handle the kill system call
        case 0x06: {
            // Get the pid and signal
            uint32_t pid = ctx->gpr[0];
            uint32_t signal = ctx->gpr[1];

            // Send the signal to the process
            switch(signal) {
                // Handle the SIG_TERM signal
                case 0x00: {
                    for (int i = 0; i < PRIORITY_LEVLS; i++) {
                        deleteL(multiq[i], pid, 0);    
                    }
                    // Remove the PCB for this process from the process table
                    deleteL(ptable, pid, 1);
                    ctx->gpr[0] = 0;
                }
            }
            break;
        }
        // Handle the nice system call
        case 0x07: {
                
        }
    }
}

// Helpers - May be moved??

int exp2(int val) {
    int out = 1;
    for (int i = 0; i < val; i++) {
        out = out * 2;
    }
    return out;
}
