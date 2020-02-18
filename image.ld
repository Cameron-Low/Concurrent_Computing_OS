SECTIONS {
    /* Assign load address (per  QEMU) */
    . = 0x70010000;

    /* Place text segment(s) */
    .text : {
        kernel/lolevel.o(.text)
        *(.text .rodata)
    }

    /* Place data segment(s) */        
    .data : {
        *(.data)
    }

    /* Place bsssegment(s) */        
    .bss : {
        *(.bss)
    }

    /* Align address (per AAPCS) */
    . = ALIGN(8);

    /* Allocate stack for irq mode */
    . = . + 0x1000;
    tos_irq = .;
    
    /* Allocate stack for svc mode */
    . = . + 0x1000;  
    tos_svc = .;

    /* Allocate stack for P3 */
    . = . + 0x1000;
    tos_P3 = .;

    /* Allocate stack for P4 */
    . = . + 0x1000;
    tos_P4 = .;
}
