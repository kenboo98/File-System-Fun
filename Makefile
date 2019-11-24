FLAGS = -std=c++11 -g -Wall -Werror -pthread
fs: FileSystem.o HelperFunctions.o
	g++ $(FLAGS) -o fs FileSystem.o HelperFunctions.o

FileSystem.o: FileSystem.cc FileSystem.h
	g++ $(FLAGS) -c FileSystem.cc

HelperFunctions.o: HelperFunctions.cc HelperFunctions.h
	g++ $(FLAGS) -c HelperFunctions.cc

clean:
	rm FileSystem.o fs

compress:
	zip a3_tellambu.zip FileSystem.cc FileSystem.h README.md Makefile
