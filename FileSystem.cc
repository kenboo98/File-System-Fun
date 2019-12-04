//
// Created by ken on 2019-11-22.
//

#include "HelperFunctions.h"
#include "FileSystem.h"
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <cstring>
#include <unordered_map>
#include <sstream>
#include <iterator>
#include <vector>

using namespace std;


// Error messages
const char *ERROR_CANNOT_FIND_DISK = "Error: Cannot find disk %s\n";
const char *ERROR_NO_MOUNT = "Error: No file system is mounted\n";
const char *ERROR_INCONSISTENT_SYSTEM = "Error: File system in %s is inconsistent (error code: %d)\n";
const char *ERROR_SUPERBLOCK_FULL = "Error: Superblock in disk %s is full, cannot create %s\n";
const char *ERROR_FILE_DIR_EXISTS = "Error: File or directory %s already exists\n";
const char *ERROR_BLOCK_ALLOCATION = "Error: Cannot allocate %d on %s\n";
const char *ERROR_FILE_DOES_NOT_EXIST = "Error: File %s does not exist\n";
const char *ERROR_FILE_DIR_DOES_NOT_EXIST = "Error: File or directory %s does not exist\n";
const char *ERROR_BLOCK_NUM_DOES_NOT_EXIST = "Error: %s does not have block %d\n";
const char *ERROR_DIRECTORY_DOES_NOT_EXIST = "Error: Directory %s does not exist\n";
const char *ERROR_CANNOT_EXPAND = "Error: File %s cannot expand to size %d\n";
const char *ERROR_COMMAND = "Command Error: %s, %d\n";

fstream file_stream;
Super_block super_block;
bool mount = false;

string disk_name;
uint8_t working_dir_index = 127;
uint8_t *buffer = new uint8_t[BLOC_BYTE_SIZE];

void reopen_filestream() {
    file_stream.close();
    if (mount) {
        file_stream.open(disk_name, fstream::in | fstream::out | fstream::binary);
    }
}

void fs_mount(const char *new_disk_name) {
    // TODO close file_stream and reopen old stream on consistency failure
    Super_block new_block = Super_block();
    // If a file is already mounted, close the current file stream so another one can be mounted
    if (mount) {
        file_stream.close();
    }
    file_stream.open(new_disk_name, fstream::in | fstream::out | fstream::binary);
    if (!file_stream.good()) {
        fprintf(stderr, ERROR_CANNOT_FIND_DISK, new_disk_name);
        reopen_filestream();
        return;
    }
    file_stream.read(new_block.free_block_list, FREE_SPACE_SIZE);
    //Initialize INodes
    for (int i = 0; i < N_INODES; i++) {
        file_stream.read(new_block.inode[i].name, NAME_SIZE);
        char buffer[3];
        file_stream.read(buffer, 3);
        new_block.inode[i].used_size = (uint8_t) buffer[0];
        new_block.inode[i].start_block = (uint8_t) buffer[1];
        new_block.inode[i].dir_parent = (uint8_t) buffer[2];

    }

    // Consistency Check 1
    unordered_map<int, int> block_to_inode;
    for (int node_idx = 0; node_idx < N_INODES; node_idx++) {
        Inode inode = new_block.inode[node_idx];
        if (inode.used_size & 0x80) {
            for (int i = inode.start_block; i < inode.start_block + (inode.used_size & 0x7F); i++) {
                if (block_to_inode.count(i)) {
                    fprintf(stderr, ERROR_INCONSISTENT_SYSTEM, new_disk_name, 1);
                    reopen_filestream();
                    return;
                }
                block_to_inode[i] = node_idx;
            }
        }
    }
    for (int i = 1; i < N_BLOCKS; i++) {
        if (get_ith_bit(new_block.free_block_list, i) == 0 && block_to_inode.count(i) == 1) {
            fprintf(stderr, ERROR_INCONSISTENT_SYSTEM, new_disk_name, 1);
            reopen_filestream();
            return;
        }
        if (get_ith_bit(new_block.free_block_list, i) == 1 && block_to_inode.count(i) != 1) {
            fprintf(stderr, ERROR_INCONSISTENT_SYSTEM, new_disk_name, 1);
            reopen_filestream();
            return;
        }
    }

    // check size and start_block of directories
    int node_index = 0;
    for (Inode inode: new_block.inode) {
        // Consistency Check 2
        if(!is_file_unique(inode.dir_parent & 0x7F, new_block.inode, inode.name, node_index)){
            fprintf(stderr, ERROR_INCONSISTENT_SYSTEM, new_disk_name, 2);
            reopen_filestream();
            return;
        }
        // Consistency check 3
        if ((inode.used_size & 0x80) == 0) {
            if (inode.used_size != 0 || inode.dir_parent != 0 || inode.start_block != 0
                || inode.name[0] != 0 || inode.name[1] != 0 || inode.name[2] != 0 || inode.name[3] != 0 || inode.name[0] != 0) {
                fprintf(stderr, ERROR_INCONSISTENT_SYSTEM, new_disk_name, 3);
                reopen_filestream();
                return;
            }
        } else if (strncmp(inode.name, "\0\0\0\0\0", 5) == 0) {
            fprintf(stderr, ERROR_INCONSISTENT_SYSTEM, new_disk_name, 3);
            reopen_filestream();
            return;
        }
        // Consistency check 5
        if (inode.dir_parent >> 7 == 1) {
            if ((inode.used_size & 0x7F) != 0 || (inode.start_block & 0x7F) != 0) {
                fprintf(stderr, ERROR_INCONSISTENT_SYSTEM, new_disk_name, 5);
                reopen_filestream();
                return;
            }
        } else if (inode.used_size & 0x80) {

            // Consistency check 4
            if (inode.start_block < 1 || inode.start_block > 127) {
                fprintf(stderr, ERROR_INCONSISTENT_SYSTEM, new_disk_name, 4);
                reopen_filestream();
                return;
            }
        }
        // Consistency Check 6
        int parent = inode.dir_parent & 0x7F;
        if (parent == 126) {
            fprintf(stderr, ERROR_INCONSISTENT_SYSTEM, new_disk_name, 6);
            reopen_filestream();
        } else if (parent >= 0 && parent <= 125 && inode.used_size & 0x80) {
            if (!(new_block.inode[parent].used_size & 0x80) || !(new_block.inode[parent].dir_parent & 0x80)) {
                fprintf(stderr, ERROR_INCONSISTENT_SYSTEM, new_disk_name, 6);
                reopen_filestream();
                return;
            }
        }
        node_index++;
    }
    mount = true;
    super_block = new_block;
    disk_name = string(new_disk_name);

}

void fs_create(const char name[5], int size) {
    if (!mount) {
        fprintf(stderr, "%s", ERROR_NO_MOUNT);
        return;
    }
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


void fs_delete_recurse(const char name[5], int current_dir) {
    int node_index = name_to_index(super_block.inode, name, current_dir);

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
        clear_used_blocks(super_block.free_block_list, super_block.inode[node_index].start_block,
                          super_block.inode[node_index].used_size & 0x7F);
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

void fs_delete(const char name[5]) {
    if (!mount) {
        fprintf(stderr, "%s", ERROR_NO_MOUNT);
        return;
    }
    fs_delete_recurse(name, working_dir_index);
}


void fs_read(const char name[5], int block_num) {
    if (!mount) {
        fprintf(stderr, "%s", ERROR_NO_MOUNT);
        return;
    }
    int node_index = name_to_index(super_block.inode, name, working_dir_index);
    if (node_index == -1) {
        fprintf(stderr, ERROR_FILE_DIR_DOES_NOT_EXIST, name);
        return;
    }
    if ((super_block.inode[node_index].used_size & 0x7F) <= block_num || block_num < 0) {
        fprintf(stderr, ERROR_BLOCK_NUM_DOES_NOT_EXIST, name, block_num);
        return;
    }
    read_block(buffer, super_block.inode[node_index].start_block + block_num, file_stream);
}

void fs_write(const char name[5], int block_num) {
    if (!mount) {
        fprintf(stderr, "%s", ERROR_NO_MOUNT);
        return;
    }
    int node_index = name_to_index(super_block.inode, name, working_dir_index);
    if (node_index == -1) {
        fprintf(stderr, ERROR_FILE_DOES_NOT_EXIST, name);
        return;
    }
    if ((super_block.inode[node_index].used_size & 0x7F) <= block_num || block_num < 0) {
        fprintf(stderr, ERROR_BLOCK_NUM_DOES_NOT_EXIST, name, block_num);
        return;
    }
    write_block(buffer, super_block.inode[node_index].start_block + block_num, file_stream);


}


void fs_buff(uint8_t buff[1024]) {
    if (!mount) {
        fprintf(stderr, "%s", ERROR_NO_MOUNT);
        return;
    }
    for (int i = 0; i < BLOC_BYTE_SIZE; i++) {
        buffer[i] = 0;
    }
    strncpy(reinterpret_cast<char *>(buffer), reinterpret_cast<const char *>(buff), 1024);
}

void fs_ls() {
    if (!mount) {
        fprintf(stderr, "%s", ERROR_NO_MOUNT);
        return;
    }
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

void fs_resize(const char name[5], int new_size) {
    if (!mount) {
        fprintf(stderr, "%s", ERROR_NO_MOUNT);
        return;
    }
    int node_index = name_to_index(super_block.inode, name, working_dir_index);
    if (node_index == -1) {
        fprintf(stderr, ERROR_FILE_DOES_NOT_EXIST, name);
    }
    int old_size = super_block.inode[node_index].used_size & 0x7F;
    if (old_size > new_size) {
        for (int i = 0; i < (old_size - new_size); i++) {
            zero_out_block(super_block.inode[node_index].start_block + new_size + i, file_stream);
        }
        clear_used_blocks(super_block.free_block_list,
                          super_block.inode[node_index].start_block + new_size, old_size - new_size);
        super_block.inode[node_index].used_size = 0x80 | new_size;
    }
    if (old_size < new_size) {
        int start_block = super_block.inode[node_index].start_block;
        // check if the next blocks are free
        bool not_free = 0;
        for (int i = start_block; i < start_block + new_size; i++) {
            not_free = not_free | get_ith_bit(super_block.free_block_list, i);
        }
        if (not_free) {
            // create a copy of the free block list so you can search for the
            // next free node assuming that the old blocks are cleared
            char free_copy[16];
            memcpy(free_copy, super_block.free_block_list, 16);
            clear_used_blocks(free_copy, start_block, old_size);
            int new_start_block = free_contiguous_blocks(free_copy, new_size);
            if (new_start_block == -1) {
                fprintf(stderr, ERROR_CANNOT_EXPAND, name, new_size);
                return;
            } else {
                move_blocks(start_block, new_start_block, old_size, file_stream);
                clear_used_blocks(super_block.free_block_list, start_block, old_size);
                set_used_blocks(super_block.free_block_list, new_start_block, new_size);
                super_block.inode[node_index].start_block = new_start_block;
                super_block.inode[node_index].used_size = 0x80 | new_size;
            }

        } else {
            super_block.inode[node_index].used_size = 0x80 | new_size;
            set_used_blocks(super_block.free_block_list, start_block + old_size, new_size - old_size);
        }
    }
    write_superblock(super_block, file_stream);
}

void fs_cd(const char name[5]) {
    if (!mount) {
        fprintf(stderr, "%s", ERROR_NO_MOUNT);
        return;
    }
    if (strncmp(name, ".", 5) == 0) {
        return;
    }
    if (strncmp(name, "..", 5) == 0) {
        if (working_dir_index == 127) {
            fprintf(stderr, ERROR_DIRECTORY_DOES_NOT_EXIST, name);
            return;
        } else {
            working_dir_index = super_block.inode[working_dir_index].dir_parent & 0x7F;
            return;
        }
    }
    int node_index = name_to_index(super_block.inode, name, working_dir_index);

    if (node_index != -1 && super_block.inode[node_index].dir_parent & 0x80
        && (super_block.inode[node_index].dir_parent & 0x7F) == working_dir_index) {
        working_dir_index = node_index;
        return;
    } else {
        fprintf(stderr, ERROR_DIRECTORY_DOES_NOT_EXIST, name);
    }


}

void fs_defrag() {
    if (!mount) {
        fprintf(stderr, "%s", ERROR_NO_MOUNT);
        return;
    }
    unordered_map<int, int> block_to_inode;
    for (int node_idx = 0; node_idx < N_INODES; node_idx++) {
        Inode inode = super_block.inode[node_idx];
        if (inode.used_size & 0x80) {
            for (int i = inode.start_block; i < inode.start_block + (inode.used_size & 0x7F); i++) {
                block_to_inode[i] = node_idx;
            }
        }
    }
    int index = 1;
    int last_free_index = -1;
    while (index < N_BLOCKS) {
        if (get_ith_bit(super_block.free_block_list, index)) {
            if (last_free_index != -1) {
                int node_index = block_to_inode[index];
                int old_idx = super_block.inode[node_index].start_block;
                int size = super_block.inode[node_index].used_size & 0x7F;

                move_blocks(old_idx, last_free_index, size, file_stream);
                clear_used_blocks(super_block.free_block_list, old_idx, size);
                set_used_blocks(super_block.free_block_list, last_free_index, size);

                super_block.inode[node_index].start_block = last_free_index;

                index = last_free_index + size - 1;
                last_free_index = -1;
            }
        } else {
            if (last_free_index == -1) {
                last_free_index = index;

            }
        }
        index++;
    }
    write_superblock(super_block, file_stream);
}


void parse_file(string file_name) {
    ifstream input_file(file_name);
    string line;
    int line_num = 0;
    // TODO: verify correctness of input
    while (getline(input_file, line)) {
        line_num++;
        istringstream ss(line);
        istream_iterator<string> begin(ss), end;
        vector<string> tokens(begin, end);

        if (tokens[0] == "M") {
            if (tokens.size() != 2) {
                fprintf(stderr, ERROR_COMMAND, file_name.c_str(), line_num);
                continue;
            }
            fs_mount(tokens[1].c_str());
        } else if (tokens[0] == "C") {
            if (tokens.size() != 3) {
                fprintf(stderr, ERROR_COMMAND, file_name.c_str(), line_num);
                continue;
            }
            int size = stoi(tokens[2]);
            if (size >= N_BLOCKS - 1 || size < 0 || tokens[1].size() > 5) {
                fprintf(stderr, ERROR_COMMAND, file_name.c_str(), line_num);
                continue;
            }
            fs_create(tokens[1].c_str(), size);
        } else if (tokens[0] == "D") {
            if (tokens.size() != 2) {
                fprintf(stderr, ERROR_COMMAND, file_name.c_str(), line_num);
                continue;
            }
            if (tokens[1].size() > 5) {
                fprintf(stderr, ERROR_COMMAND, file_name.c_str(), line_num);
                continue;
            }
            fs_delete(tokens[1].c_str());
        } else if (tokens[0] == "R") {
            if (tokens.size() != 3) {
                fprintf(stderr, ERROR_COMMAND, file_name.c_str(), line_num);
                continue;
            }
            int block = stoi(tokens[2]);
            if (block < 0 || block > 127) {
                fprintf(stderr, ERROR_COMMAND, file_name.c_str(), line_num);
                continue;
            }
            fs_read(tokens[1].c_str(), block);
        } else if (tokens[0] == "W") {
            if (tokens.size() != 3) {
                fprintf(stderr, ERROR_COMMAND, file_name.c_str(), line_num);
                continue;
            }
            int block = stoi(tokens[2]);
            if (block < 0 || block > 127) {
                fprintf(stderr, ERROR_COMMAND, file_name.c_str(), line_num);
                continue;
            }
            fs_write(tokens[1].c_str(), block);
        } else if (tokens[0] == "L") {
            if (tokens.size() != 1) {
                fprintf(stderr, ERROR_COMMAND, file_name.c_str(), line_num);
                continue;
            }
            fs_ls();
        } else if (tokens[0] == "E") {
            if (tokens.size() != 3) {
                fprintf(stderr, ERROR_COMMAND, file_name.c_str(), line_num);
                continue;
            }
            int size = stoi(tokens[2]);
            if (size >= N_BLOCKS - 1 || size < 1 || tokens[1].size() > 5) {
                fprintf(stderr, ERROR_COMMAND, file_name.c_str(), line_num);
                continue;
            }
            fs_resize(tokens[1].c_str(), size);
        } else if (tokens[0] == "O") {
            if (tokens.size() != 1) {
                fprintf(stderr, ERROR_COMMAND, file_name.c_str(), line_num);
                continue;
            }
            fs_defrag();
        } else if (tokens[0] == "Y") {
            if (tokens.size() != 2) {
                fprintf(stderr, ERROR_COMMAND, file_name.c_str(), line_num);
                continue;
            }
            fs_cd(tokens[1].c_str());
        } else if (tokens[0] == "B") {
            if (tokens.size() < 2) {
                fprintf(stderr, ERROR_COMMAND, file_name.c_str(), line_num);
                continue;
            }
            uint8_t buffer[1024];
            int vector_size = tokens.size();
            string s;
            for (int i = 1; i < vector_size; i++) {
                s.append(tokens[i]);
                if (i != vector_size - 1) {
                    s.append(" ");
                }

            }
            if(s.size() > 1024){
                fprintf(stderr, ERROR_COMMAND, file_name.c_str(), line_num);
                continue;
            }
            strcpy((char *) buffer, s.c_str());
            fs_buff(buffer);
        } else {
            fprintf(stderr, ERROR_COMMAND, file_name.c_str(), line_num);
        }

    }


}

int main(int argc, char **argv) {
    parse_file(string(argv[1]));
    file_stream.close();
    delete[] buffer;

}