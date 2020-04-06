#include "file.h"

// Read an inode from the disk
void read_inode_block(int inode_num, inode_t* inode) {
    uint8_t block[BLOCK_LENGTH] = {0};
    disk_rd(inode_num + BITMAP_LENGTH, block);
    memcpy(inode, block, sizeof(inode_t));
}

// Write an inode to the disk
void write_inode_block(int inode_num, inode_t* inode) {
    uint8_t block[BLOCK_LENGTH] = {0};
    memcpy(block, inode, sizeof(inode_t));
    disk_wr(inode_num + BITMAP_LENGTH, block);
}

// Read a directory entry from the disk
void write_dir_entry(int data_block_num, dir_entry_t* dir_entry) {
    uint8_t block[BLOCK_LENGTH] = {0};
    memcpy(block, dir_entry, sizeof(dir_entry_t));
    write_data_block(data_block_num, block);
}

// Write a directory entry to the disk
void read_dir_entry(int data_block_num, dir_entry_t* dir_entry) {
    uint8_t block[BLOCK_LENGTH] = {0};
    read_data_block(data_block_num, block);
    memcpy(dir_entry, block, sizeof(dir_entry_t));
}

// Write a data block to the disk
void write_data_block(int data_block_num, uint8_t* block) {
    disk_wr(data_block_num + BITMAP_LENGTH + INODES, block);
}

// Read a data block from the disk
void read_data_block(int data_block_num, uint8_t* block) {
    disk_rd(data_block_num + BITMAP_LENGTH + INODES, block);
}

// Add data to a specific inode's data pointer
void add_inode_data(inode_t *inode, int inode_num, int ptr, uint8_t* block) {
    // Check if there was data there already, free those blocks
    if (inode->directptrs[ptr] != -1) {
        free_data_block(inode->directptrs[ptr]);
    }

    // Create a new data block and add a pointer to it
    inode->directptrs[ptr] = claim_data_block();
    write_data_block(inode->directptrs[ptr], block);
    write_inode_block(inode_num, inode);
}

// Checks the bitmap blocks to see whether the given block number is free
int check_and_claim_block(int block_num) {
    // Calculate the correct bitmap block then byte then bit for this block number
    int bm_block = block_num / 512;
    int bm_byte = (block_num % 512) / 8;
    int bm_bit = block_num % 8;

    // Load the bitmap block for the given block number
    uint8_t block[BLOCK_LENGTH] = {0};
    disk_rd(bm_block, block);
    // Check if the corresponding bit is 0 (i.e. the block is free)
    if (~block[bm_byte] & (1 << bm_bit)) {
        // Set the correct bit to indicate a taken block
        block[bm_byte] |= 1 << bm_bit;
        disk_wr(bm_block, block);
        return 1;
    }
    
    // Otherwise block wasn't free
    return 0;
}

// Free a block from the bitmap
void free_block(int block_num) {
    // Calculate the correct bitmap block then byte then bit for this block number
    int bm_block = block_num / 512;
    int bm_byte = (block_num % 512) / 8;
    int bm_bit = block_num % 8;
    
    // Clear the desired block from the bitmap
    uint8_t block[BLOCK_LENGTH] = {0};
    disk_rd(bm_block, block);
    if (bm_bit == 0) {
        block[bm_byte] &= 0xFE;
    } else {
        uint8_t mask = (0 << bm_bit) | ((1 << bm_bit) - 1);
        for (int i = bm_bit + 1; i < 8; i++) {
            mask |= (1 << i);
        }
        block[bm_byte] &= mask;
    }
    disk_wr(bm_block, block);
}

// Claim the first available inode block
int claim_inode_block() {
    for (int i = 0; i < INODES; i++) {
        if (check_and_claim_block(i)) {
            return i;
        }
    }
    return -1;
}

// Claim the first available data block
int claim_data_block() {
    for (int i = 0; i < DATA_BLOCKS; i++) {
        if (check_and_claim_block(i + INODES)) {
            return i;
        }
    }
    return -1;
}

// Wrapper around free_block specific for data blocks
void free_data_block(int block_num) {
    free_block(INODES + block_num);
}

// Wrapper around free_block specific for inode blocks
void free_inode_block(int inode_num) {
    free_block(inode_num);
}

