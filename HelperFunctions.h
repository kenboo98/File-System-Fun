//
// Created by ken on 2019-11-24.
//

#ifndef A3_HELPERFUNCTIONS_H
#define A3_HELPERFUNCTIONS_H
#include "FileSystem.h"

int free_block_index(const char free_blocks[16]);
int free_inode_index(Inode inodes[N_INODES]);
#endif //A3_HELPERFUNCTIONS_H
