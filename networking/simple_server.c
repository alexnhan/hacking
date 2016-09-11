/* Simple server that will accept requests on port 7890
 * Prints "Hello, world!" then accepts input from the client
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "hacking.h"

#define PORT 7890 // The port users will be connecting to

int main(void) {
	int sockfd, new_sockfd; // Listen on sock_fd, new connection on new_sockfd
	struct sockaddr_in host_addr, client_addr;
	socklen_t sin_size;
	int recv_length = 1, yes = 1;
	char buffer[1024];

	// create the socket and store in sockfd
	if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) 
		perror("in socket");

	// set socket option on socket level to use the address and port no matter what for binding
	// the last two arguments, tells the option of SO_REUSEADDR(sizeof int) to be set to true(1)
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		perror("setting socket option SO_REUSEADDR");

	// setup the address structure
	host_addr.sin_family = AF_INET; // Host byte order
	host_addr.sin_port = htons(PORT); // Short, network byte order
	host_addr.sin_addr.s_addr = 0;  // Automatically fill with my IP
	memset((host_addr.sin_zero), '\0', 8); // Zero the rest of the struct

	// bind socket to the current IP address on port 7890
	if(bind(sockfd, (struct sockaddr*)&host_addr, sizeof(struct sockaddr)) == -1)
		perror("binding to socket");

	// listen for connections with a max backlog size of 5
	if(listen(sockfd, 5) == -1)
		perror("listening on socket");

	// accept loop
	while(1) {
		sin_size = sizeof(struct sockaddr_in);
		if((new_sockfd = accept(sockfd, (struct sockaddr*)&client_addr, &sin_size)) == -1)
			perror("accepting connection");
		printf("server: got connection from %s port %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		send(new_sockfd, "Hello, world!\n", 14, 0);
		recv_length = recv(new_sockfd, buffer, 1024, 0);
		while(recv_length > 0) {
			printf("RECV: %d bytes\n", recv_length);
			dump(buffer, recv_length);
			recv_length = recv(new_sockfd, buffer, 1024, 0);
		}
		close(new_sockfd);
	}
	return 0;
}
