/* Make symbols available to linker */
.global lolevel_handler_rst
.global lolevel_handler_irq
.global lolevel_handler_svc

/* Handle reset interrupt */
lolevel_handler_rst:
    /* Copy IVT to correct address */
    bl int_init

    /* Initialise stack for IRQ and SVC modes (#0xD2, #0xD3 respectively) */
    msr cpsr, #0xD2
    ldr sp, =tos_irq
    msr cpsr, #0xD3
    ldr sp, =tos_svc
    
    /* Call the C code */
    bl hilevel_handler_rst

    /* Halt execution */
    b .

/* Handle IRQ interrupt */
lolevel_handler_irq:
    /* Correct return address (lr should point to the instruction we were executing rather than returning to the next instruction) */
    sub lr, lr, #4

    /* Push args, ip, lr onto stack (descending), update sp so that it points to most recently pushed item (lr) */
    stmfd sp!, {r0-r3, ip, lr}
 
    /* Call the C code */
    bl hilevel_handler_irq

    /* Pop args, ip, lr off of stack (descending), update sp so that it points to last entry */
    ldmfd sp!, {r0-r3, ip, lr}

    /* Return from interrupt */
    movs pc, lr

/* Handle SVC interrupt */
lolevel_handler_svc:
    /* Push args, ip, lr onto stack (descending), update sp so that it points to most recently pushed item (lr) */
    stmfd sp!, {r0-r3, ip, lr}

    /* Call the C code */
    bl hilevel_handler_svc

    /* Pop args, ip, lr off of stack (descending), update sp so that it points to last entry */
    ldmfd sp!, {r0-r3, ip, lr}

    /* Return from interrupt */
    movs pc, lr
