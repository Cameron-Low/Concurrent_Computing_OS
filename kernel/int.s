/* Interrupt Vector Table */
int_data:
    ldr pc, int_addr_rst    @ reset vector -> SVC mode
    b .                     @ undefined instruction vector -> UND mode
    ldr pc, int_addr_svc    @ supervisor call vector -> SVC mode
    b .                     @ pre-fetch abort vector -> ABT mode
    b .                     @ data abort vector -> ABT mode
    b .                     @ reserved
    ldr pc, int_addr_irq    @ IRQ vector -> IRQ mode
    b .                     @ FIQ vector -> FIQ mode

/* Assign values to labels for their respective interrupt handlers */
int_addr_rst:
    .word lolevel_handler_rst
int_addr_svc:
    .word lolevel_handler_svc
int_addr_irq:
    .word lolevel_handler_irq
	
/* Allow linker access */
.global int_init

/* Copy IVT to memory with base address 0 */
int_init:
    /* Destination base value */
    mov r0, #0
    
    /* Start of IVT */
    ldr r1, =int_data
    /* End of IVT */
    ldr r2, =int_init

/* Copy loop */
l0:
    /* Copy IVT entry to destination */
    ldr r3, [r1], #4
    str r3, [r0], #4

    /* Check if we have reached the end of the table */
    cmp r1, r2
    bne l0
    
    /* Return */
    mov pc, lr

/* Allow linker access */
.global int_enable_irq
.global int_unable_irq
.global int_enable_fiq
.global int_unable_fiq

/* Enable IRQ interrupts */
int_enable_irq:
    /* Clear IRQ mask bit in CPSR */
    mrs r0, cpsr
    bic r0, r0, #0x80
    msr cpsr_c, r0
    
    /* Return */
    mov pc, lr

/* Disable IRQ interrupts */
int_unable_irq:
    /* Set IRQ mask bit in CPSR */
    mrs r0, cpsr
    orr r0, r0, #0x80
    msr cpsr_c, r0

    /* Return */
    mov pc, lr

/* Enable FIQ interrupts */
int_enable_fiq:
    /* Clear FIQ mask bit in CPSR */
    mrs r0, cpsr
    bic r0, r0, #0x40
    msr cpsr_c, r0

    /* Return */
    mov pc, lr

/* Disable FIQ interrupts */
int_unable_fiq:
    /* Set FIQ mask bit in CPSR */
    mrs r0, cpsr
    orr r0, r0, #0x40
    msr cpsr_c, r0

    /* Return */
    mov pc, lr
