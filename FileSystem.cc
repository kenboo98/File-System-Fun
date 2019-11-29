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
const char *ERROR_FILE_DOES_NOT_EXIST = "Error: File %s does not exist\n";
const char *ERROR_FILE_DIR_DOES_NOT_EXIST = "Error: File or directory %s does not exist\n";
const char *ERROR_BLOCK_NUM_DOES_NOT_EXIST = "Error: %s does not have block %d\n";
const char *ERROR_DIRECTORY_DOES_NOT_EXIST = "Error Directory %s does not exist\n";

fstream file_stream;
Super_block super_block;
bool mount = false;

string disk_name;
uint8_t working_dir_index = 127;
uint8_t *buffer = new uint8_t[BLOC_BYTE_SIZE];

void fs_mount(char *new_disk_name) {
    Super_block new_block = Super_block();

    file_stream.open(new_disk_name, fstream::in | fstream::out | fstream::binary);
    file_stream.read(new_block.free_block_list, FREE_SPACE_SIZE);
    //Initialize INodes
    for (int i = 0; i < N_INODES; i++) {
        file_stream.read(new_block.inode[i].name, NAME_SIZE);
        char buffer[3];
        file_stream.read(buffer, 3);
        new_block.inode[i].used_size = (uint8_t) buffer[0];
        new_block.inode[i].start_block = (uint8_t) buffer[1];
        new_block.inode[i].dir_parent = (uint8_t) buffer[2];

        // cout << super_block.inode[i].name << endl;
    }
    // check size and start_block of directories
    for (Inode inode: new_block.inode) {
        // Consistency check 3
        if((inode.used_size & 0x80) == 0){
            if(inode.used_size != 0 || inode.dir_parent != 0 || inode.start_block != 0
                || strncmp(inode.name, "\0\0\0\0\0", 5) != 0){
                fprintf(stderr, ERROR_INCONSISTENT_SYSTEM, new_disk_name, 3);
            }
        }else{
            if( strncmp(inode.name, "\0\0\0\0\0", 5) == 0){
                fprintf(stderr, ERROR_INCONSISTENT_SYSTEM, new_disk_name, 3);
            }
        }
        // Consistency check 5
        if (inode.dir_parent >> 7 == 1) {
            if ((inode.used_size & 0x7F) != 0 || (inode.start_block & 0x7F) != 0) {
                fprintf(stderr, ERROR_INCONSISTENT_SYSTEM, new_disk_name, 5);
            }
        } else if (inode.used_size & 0x80) {

            // Consistency check 4
            if (inode.start_block < 1 || inode.start_block > 127) {
                fprintf(stderr, ERROR_INCONSISTENT_SYSTEM, new_disk_name, 4);
            }
        }
        // Consistency Check 6
        if ((inode.dir_parent & 0x7F) == 126) {
            fprintf(stderr, ERROR_INCONSISTENT_SYSTEM, new_disk_name, 6);
        } else if ((inode.dir_parent & 0x7F) > 0 && (inode.dir_parent & 0x7F) < 125) {
            int parent = inode.dir_parent & 0x7F;
            if (!(new_block.inode[parent].used_size & 80) || !(new_block.inode[parent].dir_parent & 80)) {
                fprintf(stderr, ERROR_INCONSISTENT_SYSTEM, new_disk_name, 6);
            }
        }
    }
    mount = true;
    super_block = new_block;
    disk_name = string(new_disk_name);

}

void fs_create(char name[5], int size) {
    //cout << disk_name << endl;
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
        set_used_blocks(super_block.free_block_list, start_block, size);
    }
    write_superblock(super_block, file_stream);
}


void fs_delete_recurse(char name[5], int current_dir) {
    int node_index = name_to_index(super_block.inode, name);

    if (node_index == -1 || (super_block.inode[node_index].dir_parent & 0x7F) != current_dir) {
        fprintf(stderr, ERROR_FILE_DIR_DOES_NOT_EXIST, name);
        return;
    }

    if (super_block.inode[node_index].dir_parent & 0x80) {
        for (int i = 0; i < N_INODES; i++) {
            if ((super_block.inode[i].dir_parent & 0x7F) == node_index) {
                fs_delete_recurse(super_block.inode[i].name, node_index);
            }
        }
    } else {
        for (int i = 0; i < (super_block.inode[node_index].used_size & 0x7F); i++) {
            zero_out_block(super_block.inode[node_index].start_block + i, file_stream);
        }
    }
    for (int i = 0; i < 5; i++) {
        super_block.inode[node_index].name[i] = 0;
    }
    super_block.inode[node_index].used_size = 0;
    super_block.inode[node_index].start_block = 0;
    super_block.inode[node_index].dir_parent = 0;

    write_superblock(super_block, file_stream);

}

void fs_delete(char name[5]) {
    fs_delete_recurse(name, working_dir_index);
}


void fs_read(char name[5], int block_num) {
    int node_index = name_to_index(super_block.inode, name);
    if (node_index == -1) {
        fprintf(stderr, ERROR_FILE_DIR_DOES_NOT_EXIST, name);
        return;
    }
    if ((super_block.inode[node_index].used_size & 0x7F) <= block_num || block_num < 0) {
        fprintf(stderr, ERROR_BLOCK_NUM_DOES_NOT_EXIST, name, block_num);
        return;
    }
    read_block(buffer, super_block.inode[node_index].start_block, file_stream);
}

void fs_write(char name[5], int block_num) {
    int node_index = name_to_index(super_block.inode, name);
    if (node_index == -1) {
        fprintf(stderr, ERROR_FILE_DOES_NOT_EXIST, name);
        return;
    }
    if ((super_block.inode[node_index].used_size & 0x7F) <= block_num || block_num < 0) {
        fprintf(stderr, ERROR_BLOCK_NUM_DOES_NOT_EXIST, name, block_num);
        return;
    }
    write_block(buffer, super_block.inode[node_index].start_block, file_stream);
}


void fs_buff(uint8_t buff[1024]) {
    for (int i = 0; i < BLOC_BYTE_SIZE; i++) {
        buffer[i] = 0;
    }
    strncpy(reinterpret_cast<char *>(buffer), reinterpret_cast<const char *>(buff), 1024);
}

void fs_ls() {
    if (working_dir_index == 127) {
        printf("%-5s %3d\n", ".", count_n_files(super_block.inode, 127));
        printf("%-5s %3d\n", "..", count_n_files(super_block.inode, 127));
    } else {
        printf("%-5s %3d\n", ".", count_n_files(super_block.inode, working_dir_index));
        printf("%-5s %3d\n", "..", count_n_files(super_block.inode,
                                                 super_block.inode[working_dir_index].dir_parent & 0x7F));
    }
    for (int i = 0; i < N_INODES; i++) {
        // if file is in use
        Inode *inode = &super_block.inode[i];
        if (inode->used_size & 0x80 and (inode->dir_parent & 0x7F) == working_dir_index) {
            // if directory
            string name(inode->name, 5);
            if (inode->dir_parent & 0x80) {
                printf("%-5s %3d\n", name.c_str(), count_n_files(super_block.inode, i));
            } else {
                printf("%-5s %3d KB\n", name.c_str(), inode->used_size & 0x7F);
            }
        }

    }
}

void fs_resize(char name[5], int new_size) {
    int node_index = name_to_index(super_block.inode, name);
    if (node_index == -1) {
        fprintf(stderr, ERROR_FILE_DOES_NOT_EXIST, name);
    }
    int old_size = super_block.inode[node_index].used_size & 0x7F;
    if (old_size > new_size) {
        cout << "smol" << old_size << endl;

        for (int i = 0; i < old_size; i++) {
            zero_out_block(super_block.inode[node_index].start_block + old_size + i, file_stream);
        }
    }
    // TODO: increase size
    if (old_size < new_size) {
        int start_block =  super_block.inode[node_index].start_block;
        bool not_free = 0;
        for (int i = start_block; i < start_block + old_size; i++) {
            not_free = not_free | get_ith_bit(super_block.free_block_list, i);
        }
        if(not_free){
            int new_start_block =  free_contiguous_blocks(super_block.free_block_list, new_size);
        } else {
            cout << "Resized " << endl;
            super_block.inode[node_index].used_size =  0x80 | new_size;
            set_used_blocks(super_block.free_block_list, start_block + old_size, new_size - old_size);
        }
    }
    write_superblock(super_block, file_stream);
}

void fs_cd(char name[5]) {
    if (strncmp(name, ".", 5) == 0) {
        return;
    }
    if (strncmp(name, "..", 5) == 0) {
        if (working_dir_index == 127) {
            printf(ERROR_DIRECTORY_DOES_NOT_EXIST, name);
            return;
        } else {
            working_dir_index = super_block.inode[working_dir_index].dir_parent & 0x7F;
            return;
        }
    }
    int node_index = name_to_index(super_block.inode, name);
    if (super_block.inode[node_index].dir_parent & 0x80) {
        working_dir_index = node_index;
        return;
    } else {
        printf(ERROR_DIRECTORY_DOES_NOT_EXIST, name);
    }


}

int main(int argc, char **argv) {
    fs_mount((char *) "disk0");

    fs_create((char *) "file2", 5);
    fs_resize((char *)"file2", 7);
    fs_create((char *) "dir1\0", 0);
    fs_cd((char *) "dir1\0");
    fs_create((char *) "file\0", 3);
    fs_create((char *) "file1", 1);
    //fs_delete((char *) "hi\0");
    //fs_mount((char *) "sample_tests/sample_test_4/clean_disk_result");
    //fs_create((char *) "test1", 3);
    // Test writing buffer and ls
    /*
    fs_buff((uint8_t *) "Hello My name is.\0");
    fs_write((char *) "file1", 0);
    fs_buff((uint8_t *) "GROG GROG GROG\0");
    fs_write((char *) "file\0", 2);
    fs_buff((uint8_t *) "PLEBS\0");
    fs_write((char *) "file2\0", 3);
    fs_ls();
    fs_cd((char *) "dir1\0");
    fs_ls();
    fs_create((char *) "poop\0", 1);
    fs_write((char *) "poop\0", 0);
    fs_ls();
    fs_cd((char *) "..");
    fs_ls();
    */
    /**
    fs_buff((uint8_t *) "Hello My name is.\0");
    fs_write((char *) "file\0", 0);
    fs_write((char *) "file\0", 1);
    fs_write((char *) "file\0", 2);
    //fs_resize((char *) "file\0", 1);
    fs_cd((char *) "..");
    fs_delete((char *) "dir1\0");
     */
    file_stream.close();


}