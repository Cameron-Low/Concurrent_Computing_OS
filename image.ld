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

    /* Setup kernel heap */
    .heap : {
        end = .;
        _heap_start = .;
        . = . + 0x1000;
        _heap_end = .;
    }

    /* Align address (per AAPCS) */
    . = ALIGN(8);

    /* Allocate stack for irq mode */
    . = . + 0x1000;
    tos_irq = .;
    
    /* Allocate stack for svc mode */
    . = . + 0x1000;  
    tos_svc = .;

    /* Allocate stack for console */
    . = . + 0x1000;
    tos_Console = .;

    /* Allocate stack for P3 */
    . = . + 0x1000;
    tos_P3 = .;

    /* Allocate stack for P4 */
    . = . + 0x1000;
    tos_P4 = .;

    /* Allocate stack for P5 */
    . = . + 0x1000;
    tos_P5 = .;

    /* Allocate stack for Dining */
    . = . + 0x1000000;
    tos_Dining = .;
}
