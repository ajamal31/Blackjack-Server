/* Name: Arun Jamal
 * Description: Functions that build the server
 */

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#define DEFAULT_PORT "4420"

int get_socket()
{
	struct addrinfo hints, *results, *cur;

	// Initialize hints
	memset(&hints, 0, sizeof(hints));
	// Set options for a UDP server
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	// Get avaliable addresses
	int return_value = getaddrinfo(NULL, DEFAULT_PORT, &hints, &results);

	// Error check for getaddrinfo
	if (return_value != 0) {
		fprintf(stderr, "getaddrinfo: %s\n",
			gai_strerror(return_value));
		return 1;
	}

	for (cur = results; cur != NULL; cur = cur->ai_next) {
		printf("family: %x\n", cur->ai_family);
                printf("socktype: %x\n", cur->ai_socktype);
	}

	return 0;
}
