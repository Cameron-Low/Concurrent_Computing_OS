#ifndef __LIBC_H
#define __LIBC_H

// Standard includes
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

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

// Kill process signals
#define SIG_TERM      ( 0x00 )
#define SIG_QUIT      ( 0x01 )

// Exit statuses
#define EXIT_SUCCESS  ( 0 )
#define EXIT_FAILURE  ( 1 )

// Standard file descriptors
#define  STDIN_FILENO ( 0 )
#define STDOUT_FILENO ( 1 )
#define STDERR_FILENO ( 2 )

// Convert ASCII string x into integer r
int atoi(char* x);
// Convert integer x into ASCII string r
void itoa(char* r, int x);

// Cooperatively yield control of processor
void yield();

// Write n bytes from x to the file descriptor fd; return bytes written
int write(int fd, const void* x, size_t n);
// Read n bytes into x from the file descriptor fd; return bytes read
int read(int fd, void* x, size_t n);
// Open a file at a given path, if it doesn't exist then create a new file.
int open(const char* pathname);
// Close a file
void close(int fd);
// Delete a file
void remove(const char* pathname);
// Make a directory
void mkdir(const char* pathname);
// Remove an empty directory
void rmdir(const char* pathname);
// Change current directory
void chdir(const char* pathname);
// Print current directory
char* getcwd();
// List the conctents of the dir
void listdir(const char* pathname);
// Load program into memory
void* load(int fd);

// Clone process, returning 0 iff. child or > 0 iff. parent process
int fork();
// Terminate current process
void exit(int x);
// Execute a new process at address x
void exec(const void* x);

// For process identified by pid, send signal of x (Use pid=-1 to send signal to all processes except the init process)
int kill(int pid, int x);
// For process identified by pid, set priority of x (Use pid=-1 to send signal to all processes except the init process)
void nice(int pid, int x);
// List all currently running processes 
void list_procs();

// Initialise a semaphore with a given value
uint32_t* sem_init(int val);
// Deallocate a semaphore
void sem_close(uint32_t* s);

// Increment a semaphore s
void sem_post(uint32_t* s);
// Decrement a semaphore s
void sem_wait(uint32_t* s);

// Print functions for strings and integers
void print(char* s);
void printI(int i);

// Random number generation
uint32_t rand();

#endif
