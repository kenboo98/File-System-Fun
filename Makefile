FLAGS = -std=c++11 -g -Wall -Werror -pthread
fs: FileSystem.o
	g++ $(FLAGS) -o fs FileSystem.o

FileSystem.o: FileSystem.cc FileSystem.h
	g++ $(FLAGS) -c FileSystem.cc

clean:
	rm FileSystem.o fs

compress:
	zip a3_tellambu.zip FileSystem.cc FileSystem.h README.md Makefile
