CC=gcc
CFLAGS=-Wall -std=c99 -pedantic -D_POSIX_C_SOURCE=200809L -g

.PHONY: clean

blackjack: blackjack.o server.o deck.o
	$(CC) $(CFLAGS) blackjack.o server.o deck.o -o blackjack

blackjack.o: blackjack.c server.h

server: server.c server.h

deck: deck.c server.h

clean:
	$(RM) blackjack blackjack.o server.o deck.o
