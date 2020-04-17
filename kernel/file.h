#ifndef __FILE_H
#define __FILE_H

// Standard includes
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

// Disk driver
#include "disk.h"

// Useful constants
#define BLOCK_LENGTH (64)
#define BITMAP_LENGTH (32)
#define INODES (128)
#define DATA_BLOCKS (16224)

// File types
typedef enum {
    DIRECTORY,
    DATA
} file_t;

// File access types
typedef enum {
    READ,
    WRITE,
    APPEND
} access_t;

// A directory entry pointed to by a directory inode
typedef struct {
    uint32_t inode_num;
    file_t type;
    char name[60];
} dir_entry_t;

// Simple inode containing a type and pointers to data
typedef struct {
    file_t type;
    uint32_t directptrs[12];
} inode_t;

// File control block
typedef struct {
    int fd;
    int inode_num;
    access_t access;
} fcb_t;

// Functions for read/write of specific data types to the disk
void read_inode_block(int inode_num, inode_t* inode);
void read_data_block(int data_block_num, uint8_t* block);
void read_dir_entry(int data_block_num, dir_entry_t* dir_entry);
void write_inode_block(int inode_num, inode_t* inode);
void write_data_block(int data_block_num, uint8_t* block);
void write_dir_entry(int data_block_num, dir_entry_t* dir_entry);
void add_inode_data(inode_t *inode, int inode_num, int ptr, uint8_t* block); 

// Functions for claiming inode/data blocks
int claim_inode_block();
int claim_data_block();

// Freeing inode/data blocks
void free_data_block(int data_block_num);
void free_inode_block(int inode_num);

#endif
