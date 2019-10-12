.RECIPEPREFIX +=
CC = g++
CFLAGS = -Wall -lm -lrt -lwiringPi -std=c++11 -pthread

PROG = bin/*
OBJS = obj/*

default:
    mkdir -p bin obj
    $(CC) $(CFLAGS) -c src/main.cpp -o obj/main
    $(CC) $(CFLAGS) obj/main -o bin/main

run:
    sudo ./bin/main

clean:
    rm $(PROG) $(OBJS)
