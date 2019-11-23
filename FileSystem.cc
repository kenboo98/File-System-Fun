//
// Created by ken on 2019-11-22.
//
#include "FileSystem.h"
#include <fstream>
#include <iostream>

const int FREE_SPACE_SIZE = 16;

using namespace std;
fstream file_stream;

void fs_mount(char *new_disk_name){
    file_stream.open(new_disk_name, fstream::in|fstream::out|fstream::app);
    char free_space[FREE_SPACE_SIZE];
    file_stream.read(free_space, FREE_SPACE_SIZE);
    file_stream.seekg(0, std::fstream::beg);
    for(int i = 0; i < FREE_SPACE_SIZE; i++){
        cout << free_space[i] << endl;
    }

    file_stream.close();
}

int main(int argc, char **argv){
    fs_mount((char *) "disk0");
}