/* Name: Arun Jamal
 * Description: Functions that build the server
 */

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "server.h"

// Checks if memory allocation was a success or a fail
static void mem_check(void *mem)
{
	// If the mem is NULL then the malloc failed, exit the program.
	if (mem == NULL) {
		perror("malloc fail");
		exit(EXIT_FAILURE);
	}
}

static void print_packet(char *packet)
{
	for (int i = 0; i < 320; i++) {
		if (packet[i] == 0) {
			printf("%c", '0');
		} else if (packet[i] == 1) {
			printf("%c", '1');
		} else {
			printf("%c", packet[i]);
		}
	}
	printf("\n");
}

// you might not be storing this correctly because you need to consider the
// different data types like unsigned int and char and stuff
static void store_four_bytes(uint32_t data, char *packet, int index)
{
	// Store the bytes (see if you can understand why this is working)
	packet[index] = (data >> 24) | packet[index];
	packet[index + 1] = (data >> 16) | packet[index + 1];
	packet[index + 2] = (data >> 8) | packet[index + 2];
	packet[index + 3] = data | packet[index + 3];
}

// same thing here as above
static void store_two_bytes(uint16_t data, char *packet, int index)
{
	// Store the bytes (see if you can understand why this is working)
	packet[index] = (data >> 8) | packet[index];
	packet[index + 1] = data | packet[index + 1];
	// packet[index + 2] = (data >> 8) | packet[index + 2];
	// packet[index + 3] = data | packet[index + 3];
}

// Stores the player's cards/hand in the datagram.
static void store_player_cards(char *cards, char *packet, int index)
{
	// Store one player's cards in the datagram.
	int cards_amount = strlen(cards);
	for (int i = 0; i < cards_amount; i++) {
		packet[index++] = cards[i];
	}
}

// Stores the player's information in the datagram.
// This isn't complete, refactor this and also you're only for the first 2ß
// players
static void store_player(struct Player *p, char *packet, int index)
{
	// 1 because I stored the the whole connect request in the struct so
	// ignore the first byte
	int character_tracker = 1;
	char *username = p->username;
	int start_byte = 33 + (41 * index);

	// Store the username in the datagram.
	for (int i = start_byte; i < start_byte + strlen(username); i++)
		packet[i] = username[character_tracker++];

	// Store the other bytes for the player
	store_four_bytes(p->bank, packet, start_byte + 12);
	store_four_bytes(p->bet, packet, start_byte + 16);
	store_player_cards(p->cards, packet, start_byte + 20);
}

// Stores the dealer's card in the datagram
static void store_dealer_cards(char *dealer_cards, char *packet)
{
	int byte_tracker = 12;

	// Store the dealer's card in the datagram.
	for (int i = 0; i < MAX_CARDS; i++) {
		if (dealer_cards[i] != 0) {
			packet[byte_tracker] = dealer_cards[i];
			byte_tracker++;
		} else {
			break;
		}
	}
}

// Initializes the datagram (all 0s)
static char *make_packet(char *type)
{
	char *packet = malloc(STATE_SIZE);
	mem_check(packet);
	memset(packet, 0, STATE_SIZE);
	return packet;
	// if (strncasecmp(type, "state", STATE_SIZE) == 0) {
	// 	packet = malloc(STATE_SIZE);
	// 	mem_check(packet);
	// 	memset(packet, 0, STATE_SIZE);
	// 	return packet;
	// }
}

// This function updates the datagram with the current state of the game into
// bytes (datagram/packet)
static void update_packet(struct BlackJack *game, char *packet)
{
	// Clear the packet
	memset(packet, 0, 320);

	// Store the necessary data in the the datagram.
	packet[0] = game->op_code;
	store_four_bytes(game->response_arg, packet, 1);
	store_two_bytes(game->seq_num, packet, 5);
	store_four_bytes(game->min_bet, packet, 7);
	packet[11] = game->active_player;
	store_dealer_cards(game->dealer_cards, packet);

	char *username;

	// Store the usernames in the the datagram.
	for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
		username = game->players[i]->username;
		if (strncasecmp(username, "", USERNAME_LEN) != 0) {
			store_player(game->players[i], packet, i);
		}
	}
}

// Checks if the username passed is valid or not
static int is_username_valid(char username[])
{
	// Start at index 1 because index 0 is the op code
	for (int i = 1; i < strlen(username); i++) {
		if (!isalnum(username[i])) {
			return 1;
		}
	}
	return 0;
}

// Draws a card from the deck and adds it to the dealer or the player's hand (in
// the struct BlackJack * game)
// Return: the card drawn
static char draw(struct BlackJack *game, int index)
{
	char card = draw_card(game->cards);
	int position;

	if (index == DEALER) {
		// Store the dealers cards.
		position = strlen(game->dealer_cards);
		game->dealer_cards[position] = card;
	} else {
		// Store the players cards
		position = strlen(game->players[index]->cards);
		game->players[index]->cards[position] = card;
	}

	return card;
}

static void send_error_packet(struct BlackJack *game,
			      struct sockaddr_storage addr, int error_type)
{
	int socketfd = game->socketfd;
	char *error_packet = malloc(ERROR_SIZE);
	mem_check(error_packet);
	error_packet[0] = 6;
	error_packet[1] = error_type;

	if (error_type == NO_MONEY) {
		int current_player = game->active_player;
		printf("current_player: %d\n", current_player);
	}

	int byte_sent = sendto(socketfd, error_packet, ERROR_SIZE, 0,
			       (struct sockaddr *)&addr, sizeof(addr));

	if (byte_sent == -1) {
		perror("sentto error");
	}

	free(error_packet);
}

static void send_packet(struct BlackJack *game, char *state_packet)
{
	update_packet(game, state_packet);
	int socketfd = game->socketfd;
	for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
		if (strncasecmp(game->players[i]->username, "", USERNAME_LEN) !=
		    0) {
			struct sockaddr_storage their_addr =
			    game->players[i]->address;
			socklen_t addr_len = game->players[i]->address_len;
			int bytes_sent =
			    sendto(socketfd, state_packet, STATE_SIZE, 0,
				   (struct sockaddr *)&their_addr, addr_len);
			if (bytes_sent == -1) {
				perror("bytes_sent");
				continue;
			}
		}
	}
}

static struct History *create_node(struct Player *player)
{
	struct History *history = malloc(sizeof(struct History));
	mem_check(history);
	memset(history, 0, sizeof(struct History));

	history->player = malloc(sizeof(struct Player));
	mem_check(history->player);
	strncpy(history->player->username, player->username, USERNAME_LEN);
	history->player->bank = player->bank;

	history->next = NULL;

	printf("history->player.username: %s\n", history->player->username);
	return history;
}

static void print_history(struct BlackJack *game)
{
	printf("History: \n");
	struct History *cur = game->history;
	while (cur != NULL) {
		printf("player: %s, bank: %d\n", cur->player->username,
		       cur->player->bank);
		cur = cur->next;
	}
}

static void add_history(struct BlackJack *game, struct History *history)
{

	struct History *cur = game->history;
	if (cur == NULL) {
		game->history = history;
		return;
	}

	// Update if the first node is the user
	if (strncasecmp(cur->player->username, history->player->username,
			USERNAME_LEN) == 0) {
		cur->player->bank = history->player->bank;
		free(history->player);
		free(history);
		return;
	}

	// Find the next avaliable node to add to or update the exisiting user
	while (cur->next != NULL) {
		if (strncasecmp(cur->player->username,
				history->player->username, USERNAME_LEN) == 0) {
			cur->player->bank = history->player->bank;
			free(history->player);
			free(history);
			return;
		}
		cur = cur->next;
	}
	cur->next = history;
}

static int is_broke(struct BlackJack *game, char *username)
{
	struct History *cur = game->history;

	printf("%s\n", "history in is_broke:");
	print_history(game);
	printf("\n");
	printf("username: %s\n", username);
	if (cur != NULL &&
	    strncasecmp(cur->player->username, username, USERNAME_LEN) == 0) {
		if (cur->player->bank <= 0) {
			return 0;
		} else
			printf("cur->player->username: %s\n",
			       cur->player->username);
		printf("%s is in the history\n", cur->player->username);
	}

	while (cur->next != NULL) {
		if (strncasecmp(cur->player->username, username,
				USERNAME_LEN) == 0) {
		}
	}

	return 0;
}

// THIS FUNCTION IS NOT DONE because you're all checking if the first player is
// the player that you're looking for
struct History *find_player_history(struct BlackJack *game, char *username)
{
	struct History *history = game->history;

	// If there isn't a history of the players, it means that the linked
	// list is empty;
	if (history == NULL) {
		return NULL;
	}

	if (strncasecmp(history->player->username, username, USERNAME_LEN) ==
	    0) {
		printf("history found for: %s, bank %d\n", username,
		       history->player->bank);
		return history;
	}

	return NULL;
}

// Gets the username from the packet and stores in struct BlackJack game
// THIS ISN't DONE EITHER
static void add_player(struct BlackJack *game, char packet[],
		       struct sockaddr_storage addr, char *state_packet)
{
	int seat_count = 0;
	char *username;

	printf("%s's history\n", packet);
	struct History *h = find_player_history(game, packet);
	if (h != NULL && (h->player->bank == 0)) {
		send_error_packet(game, addr, NO_MONEY);
		printf("%s history: %d\n", packet, h->player->bank);
		return;
	}

	// Find an empty seat and add the player to it.
	for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
		seat_count++;
		username = game->players[i]->username;

		// If the username already exists on the table, send an error
		// datagram stating that the username is taken.
		if (strncasecmp(username, packet, USERNAME_LEN) == 0) {
			send_error_packet(game, addr, USERNAME_TAKEN);
			break;
		}

		// Add the player to the seat if there's an empty seat
		if (strncasecmp(username, "", USERNAME_LEN) == 0) {
			// Add the player's username
			strncpy(username, packet, USERNAME_LEN);
			// Add the player's bank
			game->players[i]->bank = DEFAULT_BANK;
			game->players[i]->address = addr;
			game->players[i]->address_len = sizeof(addr);
			if (game->active_player == 0) {
				game->active_player = i + 1;
				game->players[i]->in_queue = false;
			} else {
				game->players[i]->in_queue = true;
			}

			// update_packet(game, state_packet);
			send_packet(game, state_packet);
			break;
		}
	} // End of for loop

	// Table is full, send an error datagram to the client
	if (seat_count > NUMBER_OF_PLAYERS) {
		send_error_packet(game, addr, NO_ROOM);
	}
}

static int find_next_player(struct BlackJack *game, int current_player,
			    int type)
{
	struct Player *p = game->players[game->active_player - 1];
	// current_player += 1;
	int i = current_player;
	int next_player = 0;
	int count = 1;

	if ( i != -1)
		printf("p->username(active player): %s\n", p->username);

	for (; i < NUMBER_OF_PLAYERS - 1; i++) {

		// Store the next player
		p = game->players[i + 1];
		printf("p->username(next player): %s\n", p->username);

		if (strncasecmp(p->username, "", USERNAME_LEN != 0) &&
		    p->in_queue == false) {
			printf("player next player: %s \n", p->username);
			next_player= i + 1;
			return next_player;
			// break;
		}

		// // We've went through every player now it's time for the
		// dealer
		// // to be active
		// printf("count: %d\n", count);
		// if (count == NUMBER_OF_PLAYERS) {
		// 	if (type == BET) {
		// 		next_player = 0;
		// 		break;
		// 	} else {
		// 		next_player = -1;
		// 		break;
		// 	}
		// }
		// count++;
		//
		// if (i + 1 == NUMBER_OF_PLAYERS) {
		// 	i = -1;
		// }
		//
		// p = game->players[i + 1];
		//
		// // If the next player is avaliable to bet, return their index
		// if (strncasecmp(p->username, "",
		// 		USERNAME_LEN != 0 && p->in_queue == false)) {
		// 	printf("player at %d\n", i + 1);
		// 	next_player = i + 1;
		// 	break;
		// }
	}

	return 7;
}

// Gets the bet that the player has entered in the client.
static void bet(struct BlackJack *game, char packet[], char *state_packet)
{
	uint32_t bet_amount = 0;

	bet_amount = bet_amount | (unsigned char)packet[1];
	bet_amount = bet_amount << 24;

	bet_amount = bet_amount | (unsigned char)packet[2];
	bet_amount = bet_amount << 16;

	bet_amount = bet_amount | (unsigned char)packet[3];
	bet_amount = bet_amount << 8;

	bet_amount = bet_amount | (unsigned char)packet[4];

	int current_player = game->active_player - 1;
	uint32_t current_bank = game->players[current_player]->bank;

	// printf("current_bet: %d, current_bank: %d\n", bet_amount,
	// current_bank);

	// If the player has enough money in the bank account place their bet
	// and give them 2 cards
	int next_player = find_next_player(game, current_player, BET);
	printf("next_player in bet function: %d\n", next_player);
	if (current_bank >= bet_amount && next_player <= 7) {
		game->players[current_player]->bet = bet_amount;
		game->players[current_player]->bank -= bet_amount;
		// char card = draw(game, current_player);
		// game->players[current_player]->hand_value +=
		// card_value(card); card = draw(game, current_player);
		// game->players[current_player]->hand_value += card_value(card
		if (next_player == 7) {
			// draw the cards here.
			game->active_player =
			    find_next_player(game, -1, BET) + 1;
		} else {
			game->active_player =
			    find_next_player(game, current_player, BET) + 1;
		}
		printf("active_player in bet request: %d\n",
		       game->active_player);
	}
	// Everyone has placed their bets so loop back to first player
	// else {
	// 	game->active_player = find_next_player(game, -1, BET) + 1;
	// }

	send_packet(game, state_packet);
}

static void hit(struct BlackJack *game, char *state_packet)
{
	char card;
	int current_player = (game->active_player) - 1;
	struct Player *p = game->players[current_player];

	card = draw(game, current_player);
	p->hand_value += card_value(card);

	printf("players: %d's hand_value: %d\n", current_player, p->hand_value);

	// The player lost that round so take the money out of the player's bank
	// and reset their player
	if (p->hand_value > 21) {
		p->bet = 0;
		game->active_player =
		    find_next_player(game, current_player, HIT) + 1;
	}

	// The player is out of money, they're done
	if (p->bank == 0) {
		send_error_packet(game, p->address, NO_MONEY);
		// NEED TO ADD THE PLAYER TO THE HISTORY HERE
		memset(p, 0, sizeof(struct Player));
	} else {
		// update_packet(game, state_packet);
		send_packet(game, state_packet);
	}
}

static void update_players(struct BlackJack *game)
{
	// Set all the current players' queue status to false
	for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
		struct Player *p = game->players[i];
		// printf("p->hand_value:%d, game->dealer_hand_value: %d\n",
		//        p->hand_value, game->dealer_hand_value);

		if (p->hand_value == BLACKJACK_VALUE) {
			p->bank += p->bet * 2.5;
		} else if (p->hand_value > game->dealer_hand_value) {
			p->bank += p->bet * 2;
		} else if (p->hand_value == game->dealer_hand_value) {
			p->bank += p->bet;
		}

		// Reset the player's hand
		p->hand_value = 0;
		memset(p->cards, 0, MAX_CARDS);
		p->bet = 0;

		if (strncasecmp(p->username, "", USERNAME_LEN) != 0 &&
		    p->bank == 0) {
			send_error_packet(game, p->address, NO_MONEY);
			struct History *history = history = create_node(p);
			add_history(game, history);
			memset(p->username, 0, USERNAME_LEN);
			memset(p, 0, sizeof(struct Player));
		}

		char *username = game->players[i]->username;
		if (strncasecmp(username, "", USERNAME_LEN) != 0) {
			game->players[i]->in_queue = false;
		}
	}
}

static void stand(struct BlackJack *game, char *packet, char *state_packet)
{
	int current_player = game->active_player - 1;
	int next_player = find_next_player(game, current_player, STAND);
	char card;

	printf("current_player in stand: %d\n", current_player);
	printf("next_player in stand: %d\n", next_player);

	// If the round isn't over
	if (next_player < 7) {
		game->active_player = next_player + 1;
		send_packet(game, state_packet);

		// no more players so round's over
	} else {
		printf("entered round over state!\n");

		game->active_player = 0;
		// The dealer reveals the "facedown" card.
		sleep(1);
		card = draw(game, DEALER);
		game->dealer_hand_value += card_value(card);
		send_packet(game, state_packet);
		sleep(1);

		// // Dealer draws the cards and after drawing each card the
		// dealer waits for 1 second then draws another card.
		while (game->dealer_hand_value <= DEALER_TOTAL) {
			card = draw(game, DEALER);
			game->dealer_hand_value += card_value(card);
			send_packet(game, state_packet);
			sleep(1);
		}

		sleep(1);

		// Count up all the winnings, losings, ties.. etc of the players
		// and update their balances
		update_players(game);

		send_packet(game, state_packet);

		game->active_player = find_next_player(game, -1, BET) + 1;
		game->dealer_hand_value = 0;
		memset(game->dealer_cards, 0, MAX_CARDS);
		send_packet(game, state_packet);
		sleep(1);
		card = draw(game, DEALER);
		game->dealer_hand_value += card_value(card);
		send_packet(game, state_packet);
	}
}

static void quit(struct BlackJack *game, struct sockaddr_storage addr,
		 char *packet)
{
	// https://stackoverflow.com/questions/12810587/extracting-ip-address-and-port-info-from-sockaddr-storage
	// https://msdn.microsoft.com/en-us/library/zx63b042.aspx
	struct sockaddr_in *sin = (struct sockaddr_in *)&addr;
	unsigned char *ip = (unsigned char *)&sin->sin_addr.s_addr;
	unsigned short sin_port = sin->sin_port;
	printf("sin_port %d\n", sin_port);
	int current_player = game->active_player - 1;

	for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
		struct sockaddr_in *sin2 =
		    (struct sockaddr_in *)&(game->players[i]->address);
		unsigned char *ip2 = (unsigned char *)&sin2->sin_addr.s_addr;
		unsigned short sin_port2 = sin2->sin_port;

		if (ip[0] == ip2[0] && ip[0] == ip2[0] && ip[0] == ip2[0] &&
		    ip[0] == ip2[0] && sin_port == sin_port2) {
			struct History *history = history =
			    create_node(game->players[i]);
			add_history(game, history);
			// print_history(game);
			memset(game->players[i], 0, sizeof(struct Player));
			// current_player = game->active_player;
			// game->active_player = find_next_player(game, i);
			// game->players[game->active_player -1 ]->in_queue =
			// false; printf("%s quit the game\n",
			//        game->players[i]->username);
		}
	}

	printf("History in quit request: \n");
	print_history(game);

	//
	// for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
	// 	if (strncasecmp(game->players[i]->username, "", USERNAME_LEN) !=
	// 	    0) {
	// 		game->players[i]->in_queue = false;
	// 	}
	// }

	// THIS IS PROBABLY NOT RIGHT SO YOU'LL NEED A DIFFERENT SOLUTION.
	game->active_player = find_next_player(game, current_player, BET) + 1;

	send_packet(game, packet);
}

static void handle_packet(struct BlackJack *game, char packet[],
			  struct sockaddr_storage addr, char *state_packet)
{
	// Everytime a packet is received, this needs to be updated.
	game->seq_num += 1;

	if (packet[0] == 1) {
		// printf("%s\n", "this is a connect request");
		if (is_username_valid(packet) == 0) {
			add_player(game, packet, addr, state_packet);
		} else {
			printf("%s\n", "Invalid username; can only contain "
				       "letters or digits and can't be longer "
				       "than 12 characters");
		}
	} else if (packet[0] == 2) {
		printf("this is a bet request\n");
		bet(game, packet, state_packet);
	} else if (packet[0] == 3) {
		printf("this is a stand request\n");
		stand(game, packet, state_packet);
	} else if (packet[0] == 4) {
		printf("this is a hit request\n");
		hit(game, state_packet);
	} else if (packet[0] == 5) {
		printf("this is a quit request\n");
		quit(game, addr, state_packet);
	} else if (packet[0] == 6) {
		printf("this is an error request\n");
	} else if (packet[0] == 7) {
		printf("this is a message request\n");
	} else if (packet[0] == 8) {
		printf("this is an ack request\n");
	} else {
		printf("this is not a valid request\n");
	}
}

void print_game(struct BlackJack game)
{
	printf("op_code: %d\n", game.op_code);
	printf("response_arg: %d\n", game.response_arg);
	printf("seq_num: %d\n", game.seq_num);
	printf("min_bet: %d\n", game.min_bet);
	printf("active_player: %d\n", game.active_player);
	printf("dealer_cards: %s\n", game.dealer_cards);
	printf("dealer_hand_value: %d\n", game.dealer_hand_value);
	printf("round_status: %d\n", game.round_status);
	// printf("cards: ");
	// print_deck(game.cards);
	printf("\n");

	for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
		printf("player %d = username:%s cards:%s bet:%d bank:%d "
		       "in_queue: %d\n",
		       i, game.players[i]->username, game.players[i]->cards,
		       game.players[i]->bet, game.players[i]->bank,
		       game.players[i]->in_queue);
	}
}

// Make sure you write a free function for this
static void init_game(struct BlackJack *game, int socketfd)
{
	game->socketfd = socketfd;
	game->op_code = 0;
	game->response_arg = 0;
	game->seq_num = 0;
	game->min_bet = MIN_BET;
	game->active_player = 0;
	game->round_status = NEW_ROUND;
	memset(game->dealer_cards, 0, MAX_CARDS);
	game->dealer_hand_value = 0;
	game->history = NULL;

	char *deck = make_deck(DEFAULT_NUM_OF_DECKS);
	game->cards = deck;

	// store the first card because on launch a card has to be there?
	char card = draw(game, DEALER);
	game->dealer_hand_value += card_value(card);
	printf("card_value(%d): %d\n", card, card_value(card));

	for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
		game->players[i] = malloc(sizeof(struct Player));
		mem_check(game->players[i]);
		memset(game->players[i]->username, '\0', USERNAME_LEN);
		memset(game->players[i]->cards, '\0', MAX_CARDS);
		game->players[i]->bet = 0;
		game->players[i]->bank = 0;
		game->players[i]->hand_value = 0;
		memset(&(game->players[i]->address), 0,
		       sizeof(struct sockaddr_storage));
		game->players[i]->address_len = sizeof(struct sockaddr_storage);
		game->players[i]->in_queue = false;
	}
}

// static void clear_game(struct black *game) {}

void open_connection(int socketfd)
{
	// Declare main_readfds as fd_set
	fd_set main_readfds;

	// Initializes main_readfds
	FD_ZERO(&main_readfds);

	// Adds socketfd to main_readfds pointer
	FD_SET(socketfd, &main_readfds);

	struct BlackJack game;
	struct sockaddr_storage their_addr;

	socklen_t addr_len = sizeof(their_addr);
	char *state_packet;
	char buf[500];
	int bytes_read, bytes_ready;

	// Initialize the game board
	init_game(&game, socketfd);

	state_packet = make_packet("state");

	// passed this at the same time with the state_packet and it didn't work
	// error_packet = make_packet("error");

	printf("Struct before:\n");
	print_game(game);
	printf("\n");

	// This needs to terminate when control-c is pressed
	while (1) {
		struct timeval timeout = {
		    .tv_sec = 5,
		    .tv_usec = 0, // 100 ms
		};

		// Copy the main_readfds because they will change as select is
		// checking whether the file descriptors are ready or not.
		fd_set readfds = main_readfds;

		// The number of bytes that are ready to be read.
		bytes_ready =
		    select(socketfd + 1, &readfds, NULL, NULL, &timeout);

		if (bytes_ready == -1) {
			fprintf(stderr, "%s\n", "select call failed");
			continue;
		}

		if (FD_ISSET(socketfd, &readfds)) {
			memset(buf, '\0', 500);

			bytes_read =
			    recvfrom(socketfd, buf, 500, 0,
				     (struct sockaddr *)&their_addr, &addr_len);

			if (bytes_read == -1) {
				fprintf(stderr, "%s\n",
					"server: error reading from socket");
				continue;
			}

			handle_packet(&game, buf, their_addr, state_packet);

			// update_packet(&game, state_packet);
			printf("Struct after:\n");
			print_game(game);
			printf("\n");

			// send_packet(&game, state_packet);

			printf("\n");

		} // End of if statement

		if (bytes_ready == 0) {
			// printf("%s\n", "timed out");
			// memset(buf, '\0', 500);
			//
			// bytes_read =
			//     recvfrom(socketfd, buf, 500, 0,
			// 	     (struct sockaddr *)&their_addr, &addr_len);
			//
			// if (bytes_read == -1) {
			// 	fprintf(stderr, "%s\n",
			// 		"server: error reading from socket");
			// 	continue;
			// }
			// printf("timeout buf: ");
			// print_packet(buf);
			// send_error_packet(&game, their_addr, 5);
		}

	} // End of while loop

	free(state_packet);
	close(socketfd);
}

int get_socket(char *port) // DON'T forget to close this
{
	struct addrinfo hints, *results, *cur;
	int socketfd;

	// Initialize hints
	memset(&hints, 0, sizeof(hints));

	// Set options for a UDP server
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_PASSIVE;

	// Get avaliable addresses
	// THE PORT HERE NEEDS TO BE CHANGED TO DEFAULT_PORT or whatever the
	// port is on the command line arg
	int return_value = -1;

	if (port != NULL)
		return_value = getaddrinfo(NULL, port, &hints, &results);
	else {
		return_value =
		    getaddrinfo(NULL, DEFAULT_PORT, &hints, &results);
	}

	// Error check for getaddrinfo
	if (return_value != 0) {
		fprintf(stderr, "getaddrinfo: %s\n",
			gai_strerror(return_value));
		return 1;
	}

	for (cur = results; cur != NULL; cur = cur->ai_next) {

		// Open a socket
		socketfd =
		    socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);
		if (socketfd == -1) {
			perror("Socket fail");
			continue;
		}

		int val = -1;

		// Set options for reusing an address
		return_value = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR,
					  &val, sizeof(val));
		if (return_value == -1) {
			perror("Setsockopt fail");
			close(socketfd);
			continue;
		}

		// Bind the socket to an address
		return_value = bind(socketfd, cur->ai_addr, cur->ai_addrlen);

		if (return_value == -1) {
			perror("Bind fail");
			continue;
		}

		break;
	}

	// Couldn't bind so just end here
	if (cur == NULL) {
		perror("Bind fail");
		return 2;
	}

	freeaddrinfo(results);

	return socketfd;
}
