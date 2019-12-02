#File System Simulator - CMPUT 379 Assignment 3
## Introduction
For this assignment, we have implemented a simple simulation of file system using C++. 
This implementation reads a single 128KB file implements methods to write and read from files
in the filesystem. This filesystem has a 1KB superblock at the start to define open blocks
in the file as well as the inodes for each file. An inode contains information about each
file or directory such as the name and size. We implemented basic methods for this file system 
they are explained in the section below.

To run this, create an input file with each line containing a command to run.
Then run the follow command to build and run the program.
```bash
make fs
./fs input_file.txt
```

##Implementation
The program is split up into two files `FileSystem.cc` and `Helperfuctions.cc`. `FileSystem.cc`
contains the main filesystem implementations and `HelperFunctions.cc` contains helper
functions which abstract away many common methods. All the methods check the global `mount` variable before running.
Below are all the methods implemented and the design choices.

### fs_mount
This function is the starting point for our program. This mounts the filesystem by initializing
a filestream, reading the file, initializing the superblock and performing consistency checks.
When a mounting or consistency check fails, we must stop the mounting process and print out
an error message to `stderr`. When such a failure happens, this function also reopens a filestream
if there was a filesystem that was mounted before hand. This is done in the `reopen_filestream`
helper function.

### fs_create
This function creates a new iNode and allocates the number of neccessary blocks according to the size. 
If the size is 0, a new directory is created instead of a file. When allocation happens, there are also 
various checks performed such as ensuring that there are enough blocks and ensuring that there are no
other files in the current directory witht the same name. 

### fs_delete
This function goes and deletes a file or folder. This is done thanks to the `fs_delete_recurse` helper function
which will recursively delete the child inodes in the case that a folder is deleted. In the deleting process,
an inode and its children need to be completely cleared as well. Allocated blocks also have to be cleared 
and marked as free in the super block. These sub tasks are defined in `HelperFunctions.cc` which has methods to
clear bits.

### fs_read and and fs_write
These two functions read and write blocks(not including the superblock) to and from the global buffer which is
defined at the start of the program. This method does a few checks for errors and then goes on to call
the `read_block` and `write_block` functions defined in `HelperFunctions.cc`.

### fs_buff
Writes to the global buffer using `strncpy`.

### fs_ls
Displays the files and folders in the current directory. Each file has the file size displayed while each
folder has the number of files or folders inside the file. Heavily uses the `count_n_files` helper function
to count the number of files. This function iterates through each inode and counts the number inodes which
has the dir_parent field set to the folder we're looking for. We also account for `.` and `..` separately 
since they are two more files that do not have an inode.

### fs_cd
Changes the working directory index which is set globally as the `working_dir_index`. This global variable
is set to 127 initially to represent the root index. When a `cd` happens, we simply change the `working_dir_index`
to the node index of the directory we move into. Of course, there are a few checks to ensure that the `cd` is
called to a valid directory.

### fs_resize
Resizing files is one of the more complex commands. There are two distinct cases when resizing which are shrinking
and growing a file. In the case of shrinking a file, it is much more simple as all we have to do is change the
used_size attribute, clear blocks and update the `free_block_list`. When growing, a file, there are far more
checks involved. We need to first check to see if we can increase the block size while keeping the file at the 
same start_block. We do this by taking the `free_block_list` and checking if the bits `inode.start_block + size`
to `inode.start_block + new_size` is 0. If this is not the case, we need to check if we can resize the file 
by moving the file blocks to a new start block. For this, we will make a copy of the `free_block_list` and clear
out bits in the copy. This is so we can find a new sequence of blocks with the assumption that the old sequence
of blocks have been cleared. In this case, we also have to copy blocks to its new location. 

### fs_defrag
Defragmenting file is another complex command which was difficult to implement. The basic algorithm is to 
iterate through each `free_block_list` bit, save the earliest free block index, and then when a used block 
is found, move those blocks to the earliest free block index. This is done until the we reach the end of the
`free_block_list`. This is helped with helper functions to copy blocks as well as using a map of block indexes
to inodes for fast look ups of a block's file. When the command is complete, all the files are moved to the 
the front of the filesystem with no spaces inbetween.





 
