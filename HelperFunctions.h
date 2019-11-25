//
// Created by ken on 2019-11-24.
//

#ifndef A3_HELPERFUNCTIONS_H
#define A3_HELPERFUNCTIONS_H

#include "FileSystem.h"

int free_contiguous_blocks(const char free_blocks[16], int block_size);
int free_inode_index(const Inode inodes[N_INODES]);
bool check_file_exists(uint8_t dir_index, const Inode inodes[N_INODES], const char* name);
#endif //A3_HELPERFUNCTIONS_H
