#include "file.h"

void read_inode(int inode_num, inode_t* n) {
    uint8_t block[64] = {0};
    disk_rd(inode_num + BITMAP_LENGTH, block);
    memcpy(n, block, sizeof(inode_t));
}

void write_inode(int inode_num, inode_t* n) {
    uint8_t block[64] = {0};
    memcpy(block, n, sizeof(inode_t));
    disk_wr(inode_num + BITMAP_LENGTH, block);
}

int check_block(int block_num) {
    int num = block_num;
    int bm_block = num / 512;
    int bm_byte = (num % 512) / 8;
    int bm_bit = num % 8;

    uint8_t block[64] = {0};
    disk_rd(bm_block, block);
    if (~block[bm_byte] & (1 << bm_bit)) {
        block[bm_byte] |= 1 << bm_bit;
        disk_wr(bm_block, block);
        return 1;
    } else {
        return 0;
    }
}

void free_block(int block_num) {
    int num = block_num;
    int bm_block = num / 512;
    int bm_byte = (num % 512) / 8;
    int bm_bit = num % 8;

    uint8_t block[64] = {0};
    disk_rd(bm_block, block);
    block[bm_byte] &= 0 << bm_bit;
    disk_wr(bm_block, block);
}


int get_inode() {
    for (int i = 0; i < INODES; i++) {
        if (check_block(i)) {
            return i;
        }
    }
    return -1;
}

int get_data() {
    for (int i = 0; i < DATA_BLOCKS; i++) {
        if (check_block(i + INODES)) {
            return i + INODES + BITMAP_LENGTH;
        }
    }
    return -1;
}

void free_data_block(int block_num) {
    free_block(INODES + block_num);
}

void write_dir_entry(int a, dir_entry_t* d) {
    uint8_t block[64] = {0};
    memcpy(block, d, sizeof(dir_entry_t));
    disk_wr(a, block);
}

void read_dir_entry(int a, dir_entry_t* n) {
    uint8_t block[64] = {0};
    disk_rd(a, block);
    memcpy(n, block, sizeof(dir_entry_t));
}
