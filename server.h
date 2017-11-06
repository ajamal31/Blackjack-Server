/* Name: Arun Jamal
 * Description: Function declarations for the all the functions in server.c
 */

#ifndef SERVER_H
#define SERVER_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/socket.h>

#define DEFAULT_PORT "4420"

// needs to be removed, especially after the command line
#define TEST_PORT "5629"

#define NUMBER_OF_PLAYERS 7
#define USERNAME_LEN 13
#define STATE_SIZE 320
#define ERROR_SIZE 142
#define MAX_CARDS 21
#define MIN_BET 1
#define DEFAULT_BANK 100
#define DEFAULT_NUM_OF_DECKS 2
#define DEALER 999
#define DEALER_TOTAL 16
#define BLACKJACK_VALUE 21
#define NEW_ROUND 0
#define BETS_PLACED true
#define BET 2
#define STAND 3
#define HIT 4

#define NO_MONEY 1
#define NO_ROOM 2
#define USERNAME_TAKEN 3

struct Player {
	char username[13];
	char cards[21];
	uint32_t bet;
	uint32_t bank;
	int hand_value;
	struct sockaddr_storage address;
	socklen_t address_len;
	int in_queue;
};

struct BlackJack {
	int socketfd;
	char op_code;
	uint32_t response_arg;
	uint16_t seq_num;
	uint32_t min_bet;
	char active_player;
	char dealer_cards[21];
	int dealer_hand_value;
	int round_status;
	char *cards;
	struct Player *players[7];
	struct History * history;
};

struct History {
	struct Player *player;
	struct History * next;
};

int get_socket(char *port);

void open_connection(int socketfd);

void print_game(struct BlackJack);

char *make_deck(int deck_size);

void print_deck(char *deck);

char draw_card(char *deck);

int card_value(char card);

#endif
