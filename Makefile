CC=gcc
CFLAGS=-g -lrt -Wall -Werror -fopenmp
DFLAGS=-g -lrt -Wall -fopenmp
EXE=-o run

all:
		$(CC) $(CFLAGS) $(EXE) main.c

dev:
		$(CC) $(DFLAGS) $(EXE) main.c
