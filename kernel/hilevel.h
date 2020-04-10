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

// System call identifiers
#define SYS_YIELD     ( 0x00 )
#define SYS_WRITE     ( 0x01 )
#define SYS_READ      ( 0x02 )
#define SYS_FORK      ( 0x03 )
#define SYS_EXIT      ( 0x04 )
#define SYS_EXEC      ( 0x05 )
#define SYS_KILL      ( 0x06 )
#define SYS_NICE      ( 0x07 )
#define SYS_SEM_INIT  ( 0x08 )
#define SYS_SEM_CLOSE ( 0x09 )
#define SYS_LIST_PROC ( 0x10 )
#define SYS_OPEN      ( 0x11 )
#define SYS_CLOSE     ( 0x12 )
#define SYS_REMOVE    ( 0x13 )
#define SYS_MKDIR     ( 0x14 )
#define SYS_RMDIR     ( 0x15 )
#define SYS_CHDIR     ( 0x16 )
#define SYS_GETCWD    ( 0x17 )
#define SYS_LISTDIR   ( 0x18 )
#define SYS_LOAD      ( 0x19 )
#endif
