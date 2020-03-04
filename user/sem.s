.global sem_post
.global sem_wait

sem_post:
    ldrex r1, [r0]
    add r1, r1, #1
    strex r2, r1, [r0]
    cmp r2, #0
    bne sem_post
    dmb
    mov pc, lr

sem_wait:
    ldrex r1, [r0]
    cmp r1, #0
    beq sem_wait
    sub r1, r1, #1
    strex r2, r1, [r0]
    cmp r2, #0
    bne sem_wait
    dmb
    mov pc, lr

