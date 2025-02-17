#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>

const int BLOC_BYTE_SIZE = 1024;
const int FREE_SPACE_SIZE = 16;
const int N_BLOCKS = 128;
//INode Constants
const int N_INODES = 126;
const int NAME_SIZE = 5;


typedef struct {
	char name[5];        // Name of the file or directory
	uint8_t used_size;   // Inode state and the size of the file or directory
	uint8_t start_block; // Index of the start file block
	uint8_t dir_parent;  // Inode mode and the index of the parent inode
} Inode;

typedef struct {
	char free_block_list[16];
	Inode inode[126];
} Super_block;

void fs_mount(const char *new_disk_name);
void fs_create(const char name[5], int size);
void fs_delete(const char name[5]);
void fs_read(const char name[5], int block_num);
void fs_write(const char name[5], int block_num);
void fs_buff(uint8_t buff[1024]);
void fs_ls(void);
void fs_resize(const char name[5], int new_size);
void fs_defrag(void);
void fs_cd(const char name[5]);
#ifdef __cplusplus
}
#endif

#endif
