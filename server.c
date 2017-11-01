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

#define DEFAULT_PORT "4420"
#define TEST_PORT                                                              \
	"5629" // needs to be removed, especially after the command line
	       // arguement
#define NUMBER_OF_PLAYERS 7
#define USERNAME_LEN 13
#define STATE_SIZE 320
#define MAX_CARDS 21
#define MIN_BET 1
#define DEFAULT_BANK 100

static void mem_check(void *mem)
{
	if (mem == NULL) {
		fprintf(stderr, "%s\n", "malloc fail");
		exit(EXIT_FAILURE);
	}
}

static void print_packet(char *packet)
{
	printf("packet in bytes: ");
	for (int i = 0; i < 320; i++) {
		if (packet[i] == 0) {
			printf("%c", '0');
		} else if (packet[i] == 1) {
			printf("%c", '1');
		}
		else {
			printf("%c", packet[i]);
		}
	}
	printf("\n");
}

static char *make_packet(struct black_jack *game)
{
	char *packet = malloc(320);
	mem_check(packet);
	memset(packet, 0, 320);

	// Store the min bet
	uint32_t min_bet = game->min_bet;
	packet[7] = (min_bet >> 24) | packet[7];
	packet[8] = (min_bet >> 16) | packet[8];
	packet[9] = (min_bet >> 8) | packet[9];
	packet[10] = min_bet | packet[10];

	return packet;
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

static void add_player(struct black_jack *game, char packet[])
{
	int seat_count = 0;
	// printf("packet: %s\n", packet);
	// Find an empty seat and add the player to it.
	for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
		seat_count++;

		if (strncasecmp(game->players[i]->username, packet,
				USERNAME_LEN) == 0) {
			printf("%s\n",
			       "Username is currently being "
			       "used in the game, try a different user name");
			break;
		}

		if (strncasecmp(game->players[i]->username, "", USERNAME_LEN) ==
		    0) {
			// Add the player's username
			strncpy(game->players[i]->username, packet,
				USERNAME_LEN);

			// Add the player's min_bet
			game->players[i]->min_bet = MIN_BET;

			// Add the player's bank
			game->players[i]->bank = DEFAULT_BANK;

			break;
		}
	} // End of for loop

	if (seat_count >= NUMBER_OF_PLAYERS) {
		printf("%s\n", "Table is full. Sorry, no room for you.");
	}
}

static void handle_packet(struct black_jack *game, char packet[])
{
	if (packet[0] == 1) {
		// printf("%s\n", "this is a connect request");
		if (is_username_valid(packet) == 0) {
			add_player(game, packet);
		} else {
			printf("%s\n", "Invalid username; can only contain "
				       "letters or digits and can't be longer "
				       "than 12 characters");
		}
	} else {
		printf("this is not a connect request\n");
	}
}

void print_game(struct black_jack game)
{
	printf("op_code: %d\n", game.op_code);
	printf("response_arg: %d\n", game.response_arg);
	printf("seq_num: %d\n", game.seq_num);
	printf("min_bet: %d\n", game.min_bet);
	printf("active_player: %d\n", game.active_player);
	printf("dealer_cards: %s\n", game.dealer_cards);

	for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
		printf("player %d = username:%s cards:%s min_bet:%d bank:%d\n",
		       i, game.players[i]->username, game.players[i]->cards,
		       game.players[i]->min_bet, game.players[i]->bank);
	}
}

// Make sure you write a free function for this
static void init_game(struct black_jack *game)
{
	game->op_code = 0;
	game->response_arg = 0;
	game->seq_num = 0;
	game->min_bet = MIN_BET;
	game->active_player = 0;
	memset(game->dealer_cards, 0, MAX_CARDS);

	for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
		game->players[i] = malloc(sizeof(struct player));
		mem_check(game->players[i]);
		memset(game->players[i]->username, '\0', USERNAME_LEN);
		memset(game->players[i]->cards, '\0', MAX_CARDS);
		game->players[i]->min_bet = 0;
		game->players[i]->bank = 0;
	}
}

// static void clear_game(struct black *game) {}

void open_connection(int socketfd)
{
	// init_seats(seats); // This isn't being freed right now.
	// Declare main_readfds as fd_set
	fd_set main_readfds;
	// Initializes main_readfds
	FD_ZERO(&main_readfds);
	// Adds socketfd to main_readfds pointer
	FD_SET(socketfd, &main_readfds);

	struct black_jack game;
	struct sockaddr_storage their_addr;
	socklen_t addr_len;
	char *game_packet;

	init_game(&game);

	printf("Struct before:\n");
	print_game(game);
	printf("\n");

	// This needs to terminate when control-c is pressed
	while (1) {
		struct timeval timeout = {
		    .tv_sec = 0,
		    .tv_usec = 100e3, // 100 ms
		};

		// Copy the main_readfds because they will change as select is
		// checking whether the file descriptors are ready or not.
		fd_set readfds = main_readfds;

		// The number of bytes that are ready to be read.
		int bytes_ready =
		    select(socketfd + 1, &readfds, NULL, NULL, &timeout);

		if (bytes_ready == -1) {
			fprintf(stderr, "%s\n", "select call failed");
			continue;
		}

		if (FD_ISSET(socketfd, &readfds)) {
			char buf[1500];
			memset(buf, '\0', 1500);
			addr_len = sizeof(their_addr);

			int bytes_read =
			    recvfrom(socketfd, buf, 1500, 0,
				     (struct sockaddr *)&their_addr, &addr_len);

			if (bytes_read == -1) {
				fprintf(stderr, "%s\n",
					"server: error reading from socket");
				continue;
			}

			handle_packet(&game, buf);

			printf("%s\n", "Struct current:");
			print_game(game);
			printf("\n");

			game_packet = make_packet(&game);
			print_packet(game_packet);

			char temp[320];
			memset(temp, 0, 320);
			temp[0] = 0;
			temp[7] = 1;
			temp[8] = 0;
			temp[8] = 0;
			temp[10] = 0;

			int bytes_send =
			    sendto(socketfd, game_packet, 320, 0,
				   (struct sockaddr *)&their_addr, addr_len);

			// printf("bytes_sent: %d\n", bytes_send);

			if (bytes_send == -1) {
				fprintf(stderr, "%s\n", "bytes_send error");
				continue;
			}

		} // End of if statement

		if (bytes_ready == 0) {
			// might need this later so you left it but remove if
			// it's not needed.
		}

	} // End of while loop
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
