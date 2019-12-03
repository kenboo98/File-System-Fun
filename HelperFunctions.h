//
// Created by ken on 2019-11-24.
//

#ifndef A3_HELPERFUNCTIONS_H
#define A3_HELPERFUNCTIONS_H

#include "FileSystem.h"
#include <fstream>

int free_contiguous_blocks(const char free_blocks[16], int block_size);
bool get_ith_bit(const char free_blocks[16], int i);
void set_ith_bit(char free_blocks[16], int i, bool val);
int free_inode_index(const Inode inodes[N_INODES]);
bool check_file_exists(uint8_t dir_index, const Inode inodes[N_INODES], const char* name);
bool is_file_unique(uint8_t dir_index, const Inode inodes[N_INODES], const char *name, int index);
void write_superblock(const Super_block &superBlock, std::fstream &file_stream);
int name_to_index(const Inode inodes[N_INODES], const char *name, int parent_dir);
void write_block(uint8_t buffer[1024], int block_index, std::fstream &file_stream);
void read_block(uint8_t buffer[1024], int block_index, std::fstream &file_stream);
int count_n_files(const Inode inodes[N_INODES], int dir_index);
void zero_out_block(int block_index, std::fstream &file_stream);
void set_used_blocks(char free_blocks[16], int start_index, int block_size);
void clear_used_blocks(char free_blocks[16], int start_index, int block_size);
void move_blocks(int old_start_pos, int new_start_pos, int size, std::fstream &file_stream);
#endif //A3_HELPERFUNCTIONS_H
