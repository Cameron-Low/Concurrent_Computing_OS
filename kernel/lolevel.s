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
    
    /* Create an initial placeholder 'context', with a size of 17 words (1 per register available in usr mode), on the stack */
    sub sp, sp, #68

    /* Pass the context to the high level code (R0 is in accordance with AAPCS) */
    mov r0, sp

    /* Call the C code */
    bl hilevel_handler_rst

    /* Execute initial process */
    /* Load process mode to spsr and process pc to lr */
    ldmia sp!, {r0, lr}
    msr spsr, r0
    
    /* Restore USR mode registers for process */
    ldmia sp, {r0-r12, sp, lr}^
    add sp, sp, #60    

    /* Execute process */
    movs pc, lr

/* Handle IRQ interrupt */
lolevel_handler_irq:
    /* Correct return address (lr should point to the instruction we were executing rather than returning to the next instruction) */
    sub lr, lr, #4

    /* Save current process context onto stack ready to be switched out */
    sub sp, sp, #60
    stmia sp, {r0-r12, sp, lr}^
    mrs r0, spsr
    stmdb sp!, {r0, lr}

    /* Pass the context pointer to the high level handler */
    mov r0, sp

    /* Call the C code */
    bl hilevel_handler_irq

    /* Restore the new switched-in context from the stack */
    ldmia sp!, {r0, lr}
    msr spsr, r0
    ldmia sp, {r0-r12, sp, lr}^
    add sp, sp, #60

    /* Return from interrupt */
    movs pc, lr

/* Handle SVC interrupt */
lolevel_handler_svc:
    /* Save current process context onto stack */
    sub sp, sp, #60
    stmia sp, {r0-r12, sp, lr}^
    mrs r0, spsr
    stmdb sp!, {r0, lr}
    
    /* Setup args for hi-level function */
    mov r0, sp
    ldr r1, [lr, #-4]
    bic r1, #0xFF000000
    
    /* Call the C code */
    bl hilevel_handler_svc

    /* Restore the context from the stack */
    ldmia sp!, {r0, lr}
    msr spsr, r0
    ldmia sp, {r0-r12, sp, lr}^
    add sp, sp, #60

    /* Return from interrupt */
    movs pc, lr
