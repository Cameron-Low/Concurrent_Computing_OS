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

    /* Place bss segment(s) */        
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

    /* Allocate stack for usr mode */
    /* We will split this into 4 KiB sections, 1 section will be given to each new process */
    max_procs = 21;
    proc_stack_size = 0x1000;
    . = . + proc_stack_size * max_procs;
    usr_stacks = .;
}
