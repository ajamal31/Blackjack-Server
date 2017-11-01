/* Name: Arun
 * Course: CMPT 361 (FALL 2017)
 * Assign: 2
 */

#include "server.h"
#include <arpa/inet.h> //this will probably need to remove this, used for htonl(uint32_t)
#include <stdint.h> // need to probably remove this header after, used for uint32_t;
#include <stdio.h>

int main(int argc, char const *argv[])
{
	int socketfd;
	if ((socketfd = get_socket()) != 2) {
		open_connection(socketfd);
	} else {
		fprintf(stderr, "%s\n", "Error opening a socket");
		return 1;
	}
	return 0;
}
