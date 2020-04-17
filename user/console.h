#ifndef __CONSOLE_H
#define __CONSOLE_H

// Include all standard system calls/library functions
#include "libc.h"

// Include functionality for printing back to the terminal window
#include "PL011.h"

// Useful constants
#define MAX_CMD_CHARS ( 1024 )
#define MAX_CMD_ARGS  ( 3 )

// External user programs
extern void main_P3(); 
extern void main_P4(); 
extern void main_P5(); 
extern void main_dining();

#endif
