CXX=g++
CC=gcc
CXXFLAGS=-Wall -std=c++11
CFLAGS=-Wall
TARGET=eshell
OBJ=main.o parser.o

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)
	
main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c main.cpp
	
	
parser.o: parser.c parser.h
	$(CC) $(CFLAGS) -c parser.c
	
.PHONY:clean

clean:
	rm -f $(TARGET) $(OBJ)
