CC=gcc
CFLAGS=-Wall

all: assign2.c
	$(CC) assign2.c -o assign2 $(CFLAGS)

.PHONY: clean

clean:
	$(RM) assign2
