//
// Created by ken on 2019-11-22.
//

#include "HelperFunctions.h"
#include "FileSystem.h"
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <cstring>

using namespace std;


// Error messages
const char *ERROR_INCONSISTENT_SYSTEM = "Error: File system in %s is inconsistent (error code: %d)\n";
const char *ERROR_SUPERBLOCK_FULL = "Error: Superblock in disk %s is full, cannot create %s\n";
const char *ERROR_FILE_DIR_EXISTS = "Error: File or directory %s already exists\n";
const char *ERROR_BLOCK_ALLOCATION = "Error: Cannot allocate %d blocks on %s\n";
const char *ERROR_FILE_DOES_NOT_EXIST = "Error: File or directory %s does not exist\n";
const char *ERROR_BLOCK_NUM_DOES_NOT_EXIST = "Error: %s does not have block %d\n";

fstream file_stream;
Super_block super_block;
string disk_name;
uint8_t working_dir_index = 127;
uint8_t *buffer = new uint8_t[BLOC_BYTE_SIZE];

void fs_mount(char *new_disk_name) {
    file_stream.open(new_disk_name, fstream::in | fstream::out | fstream::binary);
    file_stream.read(super_block.free_block_list, FREE_SPACE_SIZE);

    //Initialize INodes
    for (int i = 0; i < N_INODES; i++) {
        file_stream.read(super_block.inode[i].name, NAME_SIZE);
        char buffer[3];
        file_stream.read(buffer, 3);
        super_block.inode[i].used_size = (uint8_t) buffer[0];
        super_block.inode[i].start_block = (uint8_t) buffer[1];
        super_block.inode[i].dir_parent = (uint8_t) buffer[2];
        // cout << super_block.inode[i].name << endl;
    }
    // check size and start_block of directories
    for (Inode inode: super_block.inode) {
        if (inode.dir_parent >> 7 == 1) {
            if ((inode.used_size & 0x7F) != 0 || (inode.start_block & 0x7F) != 0) {
                fprintf(stderr, ERROR_INCONSISTENT_SYSTEM, new_disk_name, 5);
            }
        }
    }
    disk_name = string(new_disk_name);

}

void fs_create(char name[5], int size) {
    cout << disk_name << endl;
    int free_inode = free_inode_index(super_block.inode);
    // Check for availability of a free node
    if (free_inode == -1) {
        fprintf(stderr, ERROR_SUPERBLOCK_FULL, disk_name.c_str(), name);
        return;
    }
    // check for uniqueness of the filename
    //TODO: check for .. or .
    if (check_file_exists(working_dir_index, super_block.inode, name)) {
        fprintf(stderr, ERROR_FILE_DIR_EXISTS, name);
        return;
    }
    // check for available contiguous blocks.
    int start_block = free_contiguous_blocks(super_block.free_block_list, size);
    if (size != 0 && start_block == -1) {
        fprintf(stderr, ERROR_BLOCK_ALLOCATION, size, disk_name.c_str());
        return;
    };

    if (size == 0) {
        strcpy(super_block.inode[free_inode].name, name);
        super_block.inode[free_inode].used_size = 0x80;
        super_block.inode[free_inode].start_block = 0;
        super_block.inode[free_inode].dir_parent = 0x80 | working_dir_index;
    } else {
        strcpy(super_block.inode[free_inode].name, name);
        super_block.inode[free_inode].used_size = 0x80 | size;
        super_block.inode[free_inode].start_block = start_block;
        super_block.inode[free_inode].dir_parent = 0x7F & working_dir_index;
    }
    write_superblock(super_block, file_stream);
}

void fs_delete(char name[5]) {
    //TODO do block cleanup and recursively delete
    int node_index = name_to_index(super_block.inode, name);

    if (node_index == -1) {
        fprintf(stderr, ERROR_FILE_DOES_NOT_EXIST, name);
        return;
    }
    for(int i = 0; i<5;i++){
        super_block.inode[node_index].name[i] = 0;
    }
    super_block.inode[node_index].used_size = 0;
    super_block.inode[node_index].start_block = 0;
    super_block.inode[node_index].dir_parent = 0;

    write_superblock(super_block, file_stream);

}
void fs_read(char name[5], int block_num){
    int node_index = name_to_index(super_block.inode, name);
    if (node_index == -1) {
        fprintf(stderr, ERROR_FILE_DOES_NOT_EXIST, name);
        return;
    }
    if((super_block.inode[node_index].used_size&0x7F) <= block_num || block_num < 0){
        fprintf(stderr, ERROR_BLOCK_NUM_DOES_NOT_EXIST, name, block_num);
        return;
    }
    read_block(buffer, super_block.inode[node_index].start_block, file_stream);
}

void fs_write(char name[5], int block_num){
    int node_index = name_to_index(super_block.inode, name);
    if (node_index == -1){
        fprintf(stderr, ERROR_FILE_DOES_NOT_EXIST, name);
        return;
    }
    if((super_block.inode[node_index].used_size&0x7F) <= block_num || block_num < 0){
        fprintf(stderr, ERROR_BLOCK_NUM_DOES_NOT_EXIST, name, block_num);
        return;
    }
    write_block(buffer, super_block.inode[node_index].start_block, file_stream);
}


void fs_buff(uint8_t buff[1024]){
    for(int i = 0; i < BLOC_BYTE_SIZE; i++){
        buffer[i] = 0;
    }
    strncpy(reinterpret_cast<char *>(buffer), reinterpret_cast<const char *>(buff), 1024);
}

void fs_ls(){
    for(int i = 0; i<N_INODES; i++){
        // if file is in use
        Inode *inode = &super_block.inode[i];
        if(inode->used_size&0x80 and (inode->dir_parent&0x7F) == working_dir_index){
            // if directory
            string name(inode->name, 5);
            if (inode->dir_parent&0x80) {
                printf("%-5s %3d\n", name.c_str(), count_n_files(super_block.inode, i));
            } else {
                printf("%-5s %3d KB\n", name.c_str(), inode->used_size&0x7F);
            }
        }

    }
}

int main(int argc, char **argv) {
    fs_mount((char *) "disk0");
    fs_create((char *) "file\0", 3);
    fs_create((char *) "file1", 1);
    //fs_delete((char *) "hi\0");
    //fs_mount((char *) "sample_tests/sample_test_4/clean_disk_result");
    //fs_create((char *) "test1", 3);
    fs_buff((uint8_t *) "Hello My name is.\0");
    fs_write((char *) "file1", 0);
    cout << buffer << endl;
    fs_buff((uint8_t *) "GROG GROG GROG\0");
    fs_write((char *) "file\0", 2);
    fs_ls();
    file_stream.close();

    cout << buffer << endl;

}