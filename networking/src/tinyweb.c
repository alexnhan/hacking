#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "hacking.h"
#include "hacking-network.h"

#define PORT 80 // The port users will be connecting to
#define WEBROOT "./webroot" // The webserver's root directory

void handle_connection(int, struct sockaddr_in *); // Handle web requests
int get_file_size(int); // Returns the filesize of open file descriptor

int main(void) {
	int sockfd, new_sockfd, yes=1;
	struct sockaddr_in host_addr, client_addr;
	socklen_t sin_size;

	printf("Accepting web requets on port %d\n", PORT);

	if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
		perror("in socket");

	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		perror("setting socket option SO_REUSEADDR");

	host_addr.sin_family = AF_INET;
	host_addr.sin_port = htons(PORT);
	host_addr.sin_addr.s_addr = INADDR_ANY; // auto fill with my IP
	memset(host_addr.sin_zero, '\0', 8); // zero the rest of the struct

	if(bind(sockfd, (struct sockaddr*)&host_addr, sizeof(struct sockaddr)) == -1)
		perror("binding to socket");

	if(listen(sockfd, 20) == -1)
		perror("listening on socket");

	while(1) { // accept loop
		sin_size = sizeof(struct sockaddr_in);
		if((new_sockfd = accept(sockfd, (struct sockaddr*)&client_addr, &sin_size)) == -1)
			perror("accepting connection");
		handle_connection(new_sockfd, &client_addr);
	}
	return 0;
}

/* This function handles the connection on the passed socket from the
 * passed client address. The connection is processed as a web request,
 * and this function replies over the connected socket. Finally, the
 * passed socket is closed at the end of the function.
 */
void handle_connection(int sockfd, struct sockaddr_in* client_addr_ptr) {
	unsigned char *ptr, request[500], resource[500];
	int fd, length;

	length = recv_line(sockfd, request);

	printf("Got request from %s:%d \"%s\"\n", inet_ntoa(client_addr_ptr->sin_addr), ntohs(client_addr_ptr->sin_port), request);

	ptr = strstr(request, " HTTP/"); // Search for the valid-looking request.
	if(ptr == NULL) {
		printf(" NOT HTTP!\n");
	}
	else {
		*ptr = 0; // terminal the buffer at the endof the URL
		ptr = NULL; // set ptr to NULL
		if(strncmp(request, "GET ", 4) == 0) // GET request
			ptr = request + 4; // ptr is the URL
		if(strncmp(request, "HEAD ", 5) == 0) // HEAD request
			ptr = request + 5;

		if(ptr == NULL) { // then the request is not recognized
			printf("\tUNKNOWN REQUEST!\n");
		}
		else {
			if(ptr[strlen(ptr)-1] == '/')
				strcat(ptr, "index.html");
			strcpy(resource, WEBROOT);
			strcat(resource, ptr);
			fd = open(resource, O_RDONLY, 0);
			printf("\tOpening \'%s\'\t", resource);
			if(fd == -1) {
				printf(" 404 Not Found\n");
				send_string(sockfd, "HTTP/1.0 404 NOT FOUND\r\n");
				send_string(sockfd, "Server: Tiny webserver\r\n\r\n");
				send_string(sockfd, "<html><head><title>404 Not Found</title></head>");
				send_string(sockfd, "<body><h1>URL not found</h1></body></html>\r\n");
			}
			else {
				printf(" 200 OK\n");
				send_string(sockfd, "HTTP/1.0 200 OK\r\n");
				send_string(sockfd, "Server: Tiny webserver\r\n\r\n");
				if(ptr == request + 4) { // Then this is a GET request
					if((length = get_file_size(fd)) == -1)
						perror("getting resource file size");
					if((ptr = (unsigned char*)malloc(length)) == NULL)
						perror("allocating memory for reading resource");
					read(fd, ptr, length); // Read the file into memroy
					send(sockfd, ptr, length, 0);
					free(ptr);
				}
				close(fd);
			} // End if file not found
		} // End if block for valid request
	} // End if block for valid HTTP
	shutdown(sockfd, SHUT_RDWR); // close the socket gracefully
}

/* This function accepts an open file descriptor and returns
 * the size of the associated file. Returns -1 on failure.
 */
int get_file_size(int fd) {
	struct stat stat_struct;
	if(fstat(fd, &stat_struct) == -1)
		return -1;
	return (int) stat_struct.st_size;
}
