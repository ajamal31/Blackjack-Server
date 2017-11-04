/* Name: Arun Jamal
 * Description: Functions that build the server
 */

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
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
	if (mem == NULL) {
		fprintf(stderr, "%s\n", "malloc fail");
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
	int cards_amount = strlen(cards);
	// printf("cards_amount: %d\n", cards_amount);
	for (int i = 0; i < cards_amount; i++) {
		packet[index++] = cards[i];
	}
}

// Stores the player's information in the datagram.
// This isn't complete, refactor this and also you're only for the first 2ß
// players
static void store_player(struct player *p, char *packet, int index)
{
	// 1 because you stored the the whole connect request in the struct so
	// ignore the first byte
	int character_tracker = 1;
	char *username = p->username;
	if (index == 0) {
		for (int i = 33; i < 33 + strlen(username); i++)
			packet[i] = username[character_tracker++];
		character_tracker = 1;
		store_four_bytes(p->bank, packet, 45);
		store_four_bytes(p->bet, packet, 49);
		store_player_cards(p->cards, packet, 53);
	} else if (index == 1) {
		for (int i = 74; i < 74 + strlen(username); i++)
			packet[i] = username[character_tracker++];
		character_tracker = 1;
		store_four_bytes(p->bank, packet, 86);
		store_four_bytes(p->bet, packet, 90);
		store_player_cards(p->cards, packet, 94);
	} else if (index == 2) {
		for (int i = 115; i < 115 + strlen(username); i++)
			packet[i] = username[character_tracker++];
		character_tracker = 1;
		store_four_bytes(p->bank, packet, 127);
	} else if (index == 3) {
		for (int i = 156; i < 156 + strlen(username); i++)
			packet[i] = username[character_tracker++];
		character_tracker = 1;
		store_four_bytes(p->bank, packet, 168);
	} else if (index == 4) {
		for (int i = 197; i < 197 + strlen(username); i++)
			packet[i] = username[character_tracker++];
		character_tracker = 1;
		store_four_bytes(p->bank, packet, 209);
	} else if (index == 5) {
		for (int i = 238; i < 238 + strlen(username); i++)
			packet[i] = username[character_tracker++];
		character_tracker = 1;
		store_four_bytes(p->bank, packet, 250);
	} else if (index == 6) {
		for (int i = 279; i < 279 + strlen(username); i++)
			packet[i] = username[character_tracker++];
		character_tracker = 1;
		store_four_bytes(p->bank, packet, 291);
	} else {
		printf("%s\n", "INDEX OUT OF RANGE IN STORE_PLAYER!");
	}
}

// Stores the dealer's card in the datagram
static void store_dealer_cards(char *dealer_cards, char *packet)
{
	// printf("store_dealer_card[0]: %d\n", dealer_cards[0]);
	int byte_tracker = 12;
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
	char *packet;
	if (strncasecmp(type, "state", STATE_SIZE) == 0) {
		packet = malloc(STATE_SIZE);
		mem_check(packet);
		memset(packet, 0, STATE_SIZE);
		return packet;
	} else if (strncasecmp(type, "error", ERROR_SIZE) == 0) {
		packet = malloc(ERROR_SIZE);
		mem_check(packet);
		memset(packet, 0, ERROR_SIZE);
		return packet;
	}
	return NULL;
}

// This function updates the datagram with the current state of the game into
// bytes (datagram/packet)
static void update_packet(struct black_jack *game, char *packet)
{
	memset(packet, 0, 320);

	packet[0] = game->op_code;
	store_four_bytes(game->response_arg, packet, 1);
	store_two_bytes(game->seq_num, packet, 5);
	// Store the min bet
	store_four_bytes(game->min_bet, packet, 7);
	packet[11] = game->active_player;
	store_dealer_cards(game->dealer_cards, packet);

	char *username;
	for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
		username = game->players[i]->username;
		if (strncasecmp(username, "", USERNAME_LEN) != 0) {
			store_player(game->players[i], packet, i);
		}
	}
}

// Update the error datagram with the current status
static void update_error_packet(char *packet, int status_code)
{
	if (status_code == NO_MONEY) {
		packet[0] = 6;
		packet[1] = 1;
		// printf("YOU HAVE NO MORE MONEY TO PLAY WITH, YOU LOSE!\n");
	}
}

static int is_username_valid(char username[])
{
	// Start at index 1 because index 0 is the op code
	for (int i = 1; i < strlen(username); i++) {
		if (!isalnum(username[i])) {
			return 1;
		} else if (strlen(username) > USERNAME_LEN) {
			return 2;
		}
	}
	return 0;
}

// Draws a card from the deck and adds it to the dealer or the player's hand (in
// the struct black_jack * game)
// Return: the card drawn
static char draw(struct black_jack *game, int index)
{
	char card = draw_card(game->cards);
	int position;
	// printf("card in draw: %d\n", card);
	// 10 is used to check if the dealer is a drawing a card or not
	if (index == DEALER) {
		position = strlen(game->dealer_cards);
		game->dealer_cards[position] = card;
	} else {
		// Find out which position in the array to place the card in.
		position = strlen(game->players[index]->cards);
		// printf("position: %d\n", position);
		game->players[index]->cards[position] = card;
		// printf("game->players[index]->cards[position]: %d\n",
		//        game->players[index]->cards[position]);
	}

	return card;
}

static void send_error_packet(struct black_jack *game,
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
		// memset(game->players[current_player], 0, sizeof()
	}

	int byte_sent = sendto(socketfd, error_packet, ERROR_SIZE, 0,
			       (struct sockaddr *)&addr, sizeof(addr));

	if (byte_sent == -1) {
		perror("sentto error");
	}

	printf("%s\n", "Username is currently being "
		       "used in the game, try a different user name");
	// this might be wrong but you gootta free it right?
	free(error_packet);
}

// Gets the username from the packet and stores in struct black_jack game
static int add_player(struct black_jack *game, char packet[],
		      struct sockaddr_storage addr)
{
	int seat_count = 0;
	// printf("packet: %s\n", packet);
	char *username;

	// Find an empty seat and add the player to it.
	for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
		seat_count++;
		username = game->players[i]->username;
		if (strncasecmp(username, packet, USERNAME_LEN) == 0) {
			send_error_packet(game, addr, USERNAME_TAKEN);
			break;
		}
		if (strncasecmp(username, "", USERNAME_LEN) == 0) {
			// Add the player's username
			strncpy(username, packet, USERNAME_LEN);
			// Add the player's bank
			game->players[i]->bank = DEFAULT_BANK;
			game->players[i]->address = addr;
			if (game->active_player == 0) {
				game->active_player = i + 1;
			}
			break;
		}
	} // End of for loop

	if (seat_count > NUMBER_OF_PLAYERS) {
		send_error_packet(game, addr, NO_ROOM);
	}

	return 2000; // this needs to be changed
}

static int next_player(struct black_jack *game, int current_player)
{
	struct player *p;
	int i = current_player + 1;
	for (; i < NUMBER_OF_PLAYERS; i++) {
		p = game->players[i];
		if (strncasecmp(p->username, "", USERNAME_LEN) != 0) {
			return current_player + 1;
		}
	}
}

// Gets the bet that the player has entered in the client.
static void bet(struct black_jack *game, char packet[])
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

	printf("current_bet: %d, current_bank: %d\n", bet_amount, current_bank);

	// If the player has enough money in the bank account place their bet
	// and give them 2 cards
	if (current_bank >= bet_amount) {
		game->players[current_player]->bet = bet_amount;
		char card = draw(game, current_player);
		game->players[current_player]->hand_value += card_value(card);
		card = draw(game, current_player);
		game->players[current_player]->hand_value += card_value(card);
	} else {
		printf("you don't have enough money to bet\n");
	}

	// game->active_player = next_player(game, current_player);
}

static void hit(struct black_jack *game)
{
	char card;
	int active_player = (game->active_player) - 1;
	struct player *p = game->players[active_player];

	card = draw(game, active_player);
	p->hand_value += card_value(card);

	printf("players: %d's hand_value: %d\n", active_player, p->hand_value);

	if (p->hand_value > 21) {
		p->bank -= p->bet;
		p->bet = 0;
	}

	if (p->bank == 0) {
		send_error_packet(game, p->address, NO_MONEY);
		memset(p, 0, sizeof(struct player));
	}
}

static void stand(struct black_jack *game, char *packet)
{
	// Draw the "facedown" card;
	game->dealer_hand_value += card_value(draw(game, DEALER));

	while (game->dealer_hand_value <= DEALER_TOTAL) {
		game->dealer_hand_value += draw(game, DEALER);
	}

	int current_player = game->active_player - 1;

	struct player *p = game->players[current_player];

	if (p->hand_value == BLACKJACK_VALUE) {
		p->bank += p->bet * 1.5;
	} else if (p->hand_value >= game->dealer_hand_value) {
		p->bank += p->bet;
	}

	// Clear the dealer's hand
	memset(game->dealer_cards, 0, 21);
	game->dealer_hand_value = 0;

	// Draw a card for the next hand (round).
	char card = draw(game, DEALER);
	game->dealer_hand_value += card_value(card);

	// Clear the player's hand
	memset(p->cards, 0, 21);
	p->bet = 0;
	p->hand_value = 0;

	printf("next player: %d\n", next_player(game, current_player));

	game->active_player = next_player(game, current_player) + 1;
}

static int handle_packet(struct black_jack *game, char packet[],
			 struct sockaddr_storage addr)
{
	// Everytime a packet is received, this needs to be updated.
	game->seq_num += 1;

	if (packet[0] == 1) {
		// printf("%s\n", "this is a connect request");
		if (is_username_valid(packet) == 0) {
			return add_player(game, packet, addr);
		} else {
			printf("%s\n", "Invalid username; can only contain "
				       "letters or digits and can't be longer "
				       "than 12 characters");
		}
	} else if (packet[0] == 2) {
		bet(game, packet);
		printf("this is a bet request\n");
	} else if (packet[0] == 3) {
		printf("this is a stand request\n");
		stand(game, packet);
	} else if (packet[0] == 4) {
		printf("this is a hit request\n");
		hit(game);
	} else if (packet[0] == 5) {
		printf("this is a quit request\n");
	} else if (packet[0] == 6) {
		printf("this is an error request\n");
	} else if (packet[0] == 7) {
		printf("this is a message request\n");
	} else if (packet[0] == 8) {
		printf("this is an ack request\n");
	} else {
		printf("this is not a valid request\n");
		return 10000; // REMOVE THIS AFTER! THIS IS BAD!
	}

	return 1000; // remove this too!
}

void print_game(struct black_jack game)
{
	printf("op_code: %d\n", game.op_code);
	printf("response_arg: %d\n", game.response_arg);
	printf("seq_num: %d\n", game.seq_num);
	printf("min_bet: %d\n", game.min_bet);
	printf("active_player: %d\n", game.active_player);
	printf("dealer_cards: %s\n", game.dealer_cards);
	printf("dealer_hand_value: %d\n", game.dealer_hand_value);
	printf("cards: ");
	print_deck(game.cards);
	printf("\n");

	for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
		printf("player %d = username:%s cards:%s bet:%d bank:%d\n", i,
		       game.players[i]->username, game.players[i]->cards,
		       game.players[i]->bet, game.players[i]->bank);
	}
}

// Make sure you write a free function for this
static void init_game(struct black_jack *game, int socketfd)
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

	char *deck = make_deck(DEFAULT_NUM_OF_DECKS);
	game->cards = deck;

	// store the first card because on launch a card has to be there?
	char card = draw(game, DEALER);
	game->dealer_hand_value += card_value(card);
	printf("card_value(%d): %d\n", card, card_value(card));

	for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
		game->players[i] = malloc(sizeof(struct player));
		mem_check(game->players[i]);
		memset(game->players[i]->username, '\0', USERNAME_LEN);
		memset(game->players[i]->cards, '\0', MAX_CARDS);
		game->players[i]->bet = 0;
		game->players[i]->bank = 0;
		game->players[i]->hand_value = 0;
		memset(&(game->players[i]->address), 0,
		       sizeof(struct sockaddr_storage));
		game->players[i]->address_len = sizeof(struct sockaddr_storage);
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

	struct black_jack game;
	struct sockaddr_storage their_addr;

	socklen_t addr_len = sizeof(their_addr);
	char *state_packet;
	char buf[500];
	int bytes_sent, bytes_read, bytes_ready, return_value;

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
		    .tv_sec = 3,
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

			return_value = handle_packet(&game, buf, their_addr);
			printf("return_value: %d\n", return_value);

			update_packet(&game, state_packet);

			for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
				their_addr = game.players[i]->address;
				bytes_sent = sendto(
				    socketfd, state_packet, STATE_SIZE, 0,
				    (struct sockaddr *)&their_addr, addr_len);
				if (bytes_sent == -1) {
					perror("bytes_sent");
					continue;
				}
			}

		} // End of if statement

		if (bytes_ready == 0) {
			// printf("timeout!\n");
		}

	} // End of while loop

	free(state_packet);
	close(socketfd);
}

int get_socket() // DON'T forget to close this
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
	int return_value = getaddrinfo(NULL, TEST_PORT, &hints, &results);

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
