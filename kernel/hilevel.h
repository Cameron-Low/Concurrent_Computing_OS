#ifndef __HILEVEL_H
#define __HILEVEL_H

// Include functionality relating to newlib (the standard C library)
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Include devices that we need access to
#include "disk.h"
#include "GIC.h"
#include "PL011.h"
#include "SP804.h"

// Include functionality relating to the kernel

#include "int.h"
#include "process.h"
#include "file.h"

// Include automatic startup program
extern void* main_console;

// Useful process variables
extern int next_pid;
extern int num_procs;

// Usefule functions
void calculate_path(char* initial_path, char* added_path);
#endif
