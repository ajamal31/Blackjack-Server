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

#define DEFAULT_PORT "4420"
#define NUMBER_OF_PLAYERS 7
#define USERNAME_LEN 13
#define STATE_SIZE 320

static char *seats[NUMBER_OF_PLAYERS];
static char state[STATE_SIZE];
static struct sockaddr_storage recv_addr;
static socklen_t addr_len;

static void print_state(char state[])
{
	for (int i = 0; i < STATE_SIZE; i++) {
		printf("%c", state[i]);
	}
}

static void init_state(char state[]) { memset(state, '\0', STATE_SIZE); }

static void add_player_state(char state[], char player[])
{
	int length = strlen(player);
	int count = 74;

	for (;count < 320; count+= 41) {
		if (state[count] == '\0') {
			printf("index %d in state is null\n", count);
			break;
		}
	}

	printf("Player: ");
	for (int i = 0; i < length; i++) {
		state[count] = player[i];
		printf("%c", player[i]);
		count++;
	}
	printf("\n");

	while (length < USERNAME_LEN) {
		state[count++] = 'p';
		length++;
	}

	print_state(state);

	printf("\n");
}

// static void update_state(char state[])
// {
// 	for (int i = 7; i < STATE_SIZE; i++) {
// 		state[i] = 'a';
// 	}
// 	printf("%s\n", "state in update_state: ");
// 	print_state(state);
// }

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

static void print_table(char *table[])
{
	for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
		printf("Seat %d: %s\n", i, table[i]);
	}
}

static void add_player(char *table[], char packet[])
{
	int seat_count = 0;
	printf("packet: %s\n", packet);
	// Find an empty seat and add the player to it.
	for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
		seat_count++;

		if (strncasecmp(table[i], packet, USERNAME_LEN) == 0) {
			printf("%s\n",
			       "Username is currently being "
			       "used in the game, try a different user name");
			break;
		}

		if (strncasecmp(table[i], "", USERNAME_LEN) == 0) {
			strncpy(table[i], packet, USERNAME_LEN);
			break;
		}
	} // End of for loop

	if (seat_count >= NUMBER_OF_PLAYERS) {
		printf("%s\n", "Table is full. Sorry, no room for you.");
	}
}

static void clear_seats(char *table[])
{
	for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
		if (table[i] != NULL) {
			free(table[i]);
		}
	}
}

static void mem_check(void *mem)
{
	if (mem == NULL) {
		fprintf(stderr, "%s\n", "malloc fail");
		exit(EXIT_FAILURE);
	}
}

static void init_seats(char *table[])
{
	for (int i = 0; i < NUMBER_OF_PLAYERS; i++) {
		table[i] = malloc(USERNAME_LEN + 1);
		mem_check(table[i]);
		memset(table[i], '\0', USERNAME_LEN);
	}
}

static void handle_packet(char packet[])
{
	if (packet[0] == 1) {
		printf("%s\n", "this is a connect request");
		if (is_username_valid(packet) == 0) {
			add_player(seats, packet);
			add_player_state(state, packet);
		} else {
			printf("%s\n", "Invalid username; can only contain "
				       "letters or digits and can't be longer "
				       "than 12 characters");
		}
	} else {
		printf("this is not a connect request\n");
	}
}

void open_connection(int socketfd)
{
	init_seats(seats); // This isn't being freed right now.
	// Declare main_readfds as fd_set
	fd_set main_readfds;
	// Initializes main_readfds
	FD_ZERO(&main_readfds);
	// Adds socketfd to main_readfds pointer
	FD_SET(socketfd, &main_readfds);

	init_state(state);
	printf("state before: %s\n", state);

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
			addr_len = sizeof(recv_addr);

			int bytes_read =
			    recvfrom(socketfd, buf, 1500, 0,
				     (struct sockaddr *)&recv_addr, &addr_len);

			printf("bytes_read[0] == 1: %d\n", buf[0] == 1);

			if (bytes_read == -1) {
				fprintf(stderr, "%s\n",
					"server: error reading from socket");
				continue;
			}

			// int bytes_send =
			//     sendto(socketfd, buf, strlen(buf), 0,
			// 	   (struct sockaddr *)&recv_addr, addr_len);
			//
			// printf("bytes_sent: %d\n", bytes_send);
			//
			// if (bytes_send == -1) {
			// 	fprintf(stderr, "%s\n", "bytes_send error");
			// 	continue;
			// }

			handle_packet(buf);

			printf("%s\n", "Current table:");
			print_table(seats);
			printf("\n");

		} // End of if statement

		if (bytes_ready == 0) {
			// might need this later so you left it but remove if
			// it's not needed.
		}

	} // End of while loop

	clear_seats(seats); // this isn't doing anything right now, you'll need
			    // to add a signal handler and after control-c is
			    // pressed, kill the while loop so this funciton can
			    // run.
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
	int return_value = getaddrinfo(NULL, DEFAULT_PORT, &hints, &results);

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
