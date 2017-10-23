CC=gcc
CFLAGS=-Wall -std=c99 -pedantic -D_POSIX_C_SOURCE=200809L

all: assign2.o server.o
	$(CC) assign2.o server.o -o assign2 $(CFLAGS)

assign2: assign2.c server.h
	$(CC) assign2.c -c $(CFLAGS)

server: server.c server.h
	$(CC) server.c -c $(CFLAGS)

.PHONY: clean

clean:
	$(RM) assign2 assign2.o server.o
