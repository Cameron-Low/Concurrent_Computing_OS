#include "hilevel.h"

// Process table
ptable_t ptable;

// Currently running process
pcb_t* running = NULL;

// Perform a context switch
void dispatch(ctx_t* ctx, pcb_t* current, pcb_t* new) {
    // If there was a program running, save it's context and move it to the ready queue
    if (current != NULL) {
        memcpy(&current->ctx, ctx, sizeof(ctx_t));
        current->pstate = READY;
    }

    // If we have a new process then perform a context switch and remove process from ready queue
    if (new != NULL) {
        memcpy(ctx, &new->ctx, sizeof(ctx_t));
        new->pstate = RUNNING;
    }

    running = new;
}

// Pick the next process to execute from the ready queue
void schedule(ctx_t* ctx) {
    // Find the running process in the process table and dispatch the next ready process
    for (int n = 0; n < MAX_PROCS; n++) {
        if (ptable.table[n].pid == running->pid) {
            // Wrap around the process table
            int next = n+1 == MAX_PROCS ? 0 : n+1;

            // Dispatch next process
            dispatch(ctx, running, &ptable.table[next]);
            return;
        }
    }
}

// Hi-level code for handling RST interrupts
void hilevel_handler_rst(ctx_t* ctx) {
    // Setup timer to cause an interupt every 1 second
    TIMER0->Timer1Load = 0x100000;
    TIMER0->Timer1Ctrl = 0xE2;
    
    // Setup GIC so that timer interrupts are allowed through to the processor via IRQ
    GICC0->PMR = 0xF0;
    GICC0->CTLR = 0x1;
    GICD0->ISENABLER1 = 0x10;
    GICD0->CTLR = 0x1;

    // Initialise process table with two specified user programs
    ptable.table[0] = createPCB(&main_P3, tos_P3);
    ptable.table[1] = createPCB(&main_P4, tos_P4);

    // Add the PCBs to the ready queue
    ptable.table[0].pstate = READY;
    ptable.table[1].pstate = READY;

    // Dispatch 0th PCB entry by default
    dispatch(ctx, NULL, &ptable.table[0]);
    
    // Remove IRQ interrupt mask
    int_enable_irq();
}

// Hi-level code for handling IRQ interrupts
void hilevel_handler_irq(ctx_t* ctx) {
    // Get the id of the interrupting device
    uint32_t id = GICC0->IAR;

    switch(id) {
        case GIC_SOURCE_TIMER0: {
            PL011_putc(UART0, 'T', 1);
            // Clear the interrupt from the timer
            TIMER0->Timer1IntClr = 0x1;                        

            // Call scheduler
            schedule(ctx);
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
    }
}
