#ifndef __HILEVEL_H
#define __HILEVEL_H

// Include functionality relating to newlib (the standard C library)

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Include devices that we need access to
#include "GIC.h"
#include "PL011.h"
#include "SP804.h"

// Include functionality relating to the kernel

#include "lolevel.h"
#include "int.h"
#include "ptable.h"

// Include automatic startup programs and their associated stacks

void* main_P3;
extern uint32_t tos_P3;
void* main_P4;
extern uint32_t tos_P4;

// Function declarations
int exp2(int val);

#endif
