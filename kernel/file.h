#ifndef __FILE_H
#define __FILE_H

// Standard includes
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

// Disk driver
#include "disk.h"

// Useful constants
#define INODES (128)
#define BITMAP_LENGTH (32)
#define DATA_BLOCKS (16224)

typedef enum {
    DIRECTORY,
    DATA
} file_t;

typedef struct {
    uint32_t inode_num;
    file_t type;
    char name[10];
} dir_entry_t;

typedef struct {
    uint32_t direct_ptrs[8];
} indirect_block_t;

typedef struct {
    file_t type;
    uint32_t directptrs[12];
} inode_t;

typedef struct {
    int fd;
    int inode_num;
} fd_entry_t;

void read_inode(int a, inode_t* n);
void write_inode(int a, inode_t* n);
void read_dir_entry(int a, dir_entry_t* n);
void write_dir_entry(int a, dir_entry_t* n);

int get_inode();
int get_data();
void free_data_block(int a);

#endif
