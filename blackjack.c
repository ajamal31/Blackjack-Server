/* Main Program */

#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void usage(char c, char *arg)
{
	if (c == 'd') {
		int deck_size = atoi(arg);
		if (deck_size < 1 || deck_size > 10) {
			fprintf(stderr, "%s\n",
				"number of decks must be between 1 and 10.");
			exit(1);
		}
	}
	// else if (c == 'm') {
	// 	int bank_amount = atoi(arg);
	// 	if (bank_amount < 1 || bank_amount > 10) {
	// 		fprintf(stderr, "%s\n",
	// 			"number  must be between 1 and 10.");
	// 		exit(1);
	// 	}
	// }
}

int main(int argc, char *argv[])
{
	int c;
	char *port = NULL;
	char *deck_size = NULL;
	char *money = NULL;
	char *min_bet = NULL;

	while ((c = getopt(argc, argv, "p:d:m:b:h")) != -1) {
		switch (c) {
		case 'p':
			port = optarg;
			break;
		case 'd':
			deck_size = optarg;
			usage(c, optarg);
			break;
		case 'm':
			money = optarg;
			break;
		case 'b':
			min_bet = optarg;
			break;
		case 'h':
			printf("Usage: %s [-d number of decks] [-p port] [-m "
			       "bank amount] [-b min bet]\n default decks: 2, "
			       "default port: 4420, default min_bet: $1",
			       argv[0]);
			exit(0);

		default:
			fprintf(stderr,
				"Usage: %s [-d number of decks] [-p port] [-m "
				"bank amount] [-b min bet]\n",
				argv[0]);
			exit(1);
		}
	}

	int socketfd;
	if ((socketfd = get_socket(port)) != 2) {
		open_connection(socketfd, deck_size, money, min_bet);
	} else {
		fprintf(stderr, "%s\n", "Error opening a socket");
		return 1;
	}
	return 0;
}
