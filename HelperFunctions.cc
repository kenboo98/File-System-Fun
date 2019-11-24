//
// Created by ken on 2019-11-24.
//

#include <cstdint>
#include <string.h>
#include "FileSystem.h"

using namespace std;
/*
 * Returns the index of the next free block from the free block bytes
 * in the super block. Returns -1 if there is no free block.
 */
int free_block_index(const char free_blocks[16]){
    int index = 0;
    for(int byte_idx = 0; byte_idx<16; byte_idx++){
        for(int bit_idx = 7; bit_idx >= 0; bit_idx--){
            if( !((free_blocks[byte_idx] >> bit_idx)&1)){
                return index;
            }
            index ++;
        }
    }
    return -1;
}
/**
 * Returns the index of the next free inode.
 * @param inodes array of inodes
 * @return index of free inode or -1 if inodes are full
 */
int free_inode_index(const Inode inodes[N_INODES]){

    for(int index = 0; index < N_INODES; index++){
        if(!(inodes[index].used_size & 0x7F)){
            return index;
        }
    }
    return -1;

}

bool check_file_exists(uint8_t dir_index, const Inode inodes[N_INODES], const char* name){
    for(int i = 0; i < N_INODES; i++){
        if(dir_index == inodes[i].dir_parent &&
                strcmp(name, inodes[i].name) == 0){
            return false;
        }
    }
    return true;
}