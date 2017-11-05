/* Name: Arun
 * Course: CMPT 361 (FALL 2017)
 * Assign: 2
 */

#include "server.h"
#include <arpa/inet.h> //this will probably need to remove this, used for htonl(uint32_t)
#include <stdint.h> // need to probably remove this header after, used for uint32_t;
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	int c;
	char * port = NULL;
	while ((c = getopt(argc, argv, "p:")) != -1) {
		switch (c) {
		case 'p':
			port = optarg;
			break;
		}
	}

	printf("port: %s\n", port);

	int socketfd;
	if ((socketfd = get_socket(port)) != 2) {
		open_connection(socketfd);
	} else {
		fprintf(stderr, "%s\n", "Error opening a socket");
		return 1;
	}
	return 0;
}
