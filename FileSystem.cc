//
// Created by ken on 2019-11-22.
//
#include "FileSystem.h"
#include "HelperFunctions.h"
#include <fstream>
#include <iostream>
#include <unordered_set>

using namespace std;



// Error messages
const char* ERROR_INCONSISTENT_SYSTEM = "Error: File system in %s is inconsistent (error code: %d)\n";
const char* ERROR_SUPERBLOCK_FULL = "Error: Superblock in disk %s is full, cannot create %s";

fstream file_stream;
Super_block super_block;
string disk_name;
int working_dir_index = 127;

void fs_mount(char *new_disk_name) {
    file_stream.open(new_disk_name, fstream::in | fstream::out | fstream::app);
    file_stream.read(super_block.free_block_list, FREE_SPACE_SIZE);

    //Initialize INodes
    for (int i = 0; i < N_INODES; i++) {
        file_stream.read(super_block.inode[i].name, NAME_SIZE);
        char buffer[3];
        file_stream.read(buffer, 3);
        super_block.inode[i].used_size = (uint8_t) buffer[0];
        super_block.inode[i].start_block = (uint8_t) buffer[1];
        super_block.inode[i].dir_parent = (uint8_t) buffer[2];
        cout << super_block.inode[i].name << endl;
    }
    // check size and start_block of directories
    for (Inode inode: super_block.inode) {
        if (inode.dir_parent >> 7 == 1) {
            if ((inode.used_size&0x7F) != 0 || (inode.start_block&0x7F) != 0) {
                fprintf(stderr, ERROR_INCONSISTENT_SYSTEM, new_disk_name, 5);
            }
        }
    }
    disk_name = string(new_disk_name);
    file_stream.close();
}

void fs_create(char name[5], int size){
    int free_inode = free_inode_index(super_block.inode);
    if(free_inode == -1){
        fprintf(stderr, ERROR_SUPERBLOCK_FULL, disk_name.c_str(), name);
        return;
    }

    if(size == 0){

    }
}

int main(int argc, char **argv) {
    fs_mount((char *) "sample_tests/sample_test_4/clean_disk_result");
}