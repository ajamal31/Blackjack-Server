/* Name: Arun Jamal
 * Description: Functions that build the deck
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"

char *make_deck(int num_of_decks)
{

	int deck_size = 52 * num_of_decks;
	char *deck = malloc(deck_size + 1); // need the extra byte to store the
					    // null terminator because you're
					    // calling strlen on the deck after

	if (deck == NULL) {
		fprintf(stderr, "%s\n", "Can't allocate memory");
		exit(EXIT_FAILURE);
	}

	memset(deck, 0, deck_size + 1);

	int count = 0;

	for (int i = 0; i < num_of_decks; i++) {
		for (int j = 1; j < 53; j++) {
			deck[count] = j;
			count++;
		}
	}

	return deck;
}

void print_deck(char *deck)
{
	for (int i = 0; i < strlen(deck) - 1; i++) {
		printf("%d", deck[i]);
	}
}

char draw_card(char *deck)
{
	char card = deck[strlen(deck) - 1];
	deck[strlen(deck) - 1] = '\0';
	return card;
}
