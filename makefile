# Name: Arun Jamal
# Course: CMPT 361 (Fall 2017)
# Description: Builds the program

CC=gcc
CFLAGS=-Wall -std=c99 -pedantic -D_POSIX_C_SOURCE=200809L

.PHONY: clean

blackjack: blackjack.o server.o
	$(CC) $(CFLAGS) blackjack.o server.o -o blackjack

blackjack.o: blackjack.c server.h

server: server.c server.h

.PHONY: clean

clean:
	$(RM) blackjack blackjack.o server.o
