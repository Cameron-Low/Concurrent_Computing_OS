#ifndef __INT_H
#define __INT_H

// Include some standard type definitions
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Allow linker access to various functions.
void int_init();

void int_enable_irq();
void int_unable_irq();
void int_enable_fiq();
void int_unable_fiq();

#endif
