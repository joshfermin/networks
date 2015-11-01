#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>

#define INT_MIN 1024
#define MAX_BUFFER 2000

int listenOnPort(int);
void accept_request(int);

void accept_request(int socket)
{
	char buf[MAX_BUFFER];
	int read_size = 0, len = 0;

	while ((read_size = recv(socket, &buf[len], (MAX_BUFFER-len), 0)) > 0)
	{ 
		char line[read_size];
		strncpy(line, &buf[len], sizeof(line));
		len += read_size;
		line[read_size] = '\0';

		printf("Found:  %s\n", line);
	}
}

int listenOnPort(int port)
{
	int one = 1, client_fd, status;
	// pthread_t newthread;

	// sockaddr: structure to contain an internet address.
	struct sockaddr_in svr_addr, cli_addr;
	socklen_t sin_len = sizeof(cli_addr);

	// create endpoint for comm, AF_INET (IPv4 Internet Protocol), 
	// SOCK_STREAM (Provides sequenced, reliable, two-way, connection-based byte streams),
	int sock = socket(AF_INET, SOCK_STREAM, 0); // returns file descriptor for new socket
	if (sock < 0)
		fprintf(stderr, "Can't open socket\n");

	// (int socket SOL_SOCKET to set options at socket level, allows reuse of local addresses,
	// option value, size of socket)
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));


	svr_addr.sin_family = AF_INET;
	svr_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	svr_addr.sin_port = htons(port); // host to network short (htons)

	// Start server
		if (bind(sock, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) == -1) {
			close(sock);
			fprintf(stderr, "Can't bind socket.\n");
			// err(1, "Can't bind");
		}
	 
		listen(sock, 5); // 5 is backlog - limits amount of connections in socket listen queue
		printf("listening on localhost with port %d\n", port);


	while (1)
	{
		client_fd = accept(sock, (struct sockaddr *)&cli_addr, &sin_len);
		printf("Got connection.\n");
			if(fork() == 0){
				/* Perform the client’s request in the child process. */
				accept_request(client_fd);
				exit(0);
			}
			close(client_fd);
			waitpid(-1, &status, WNOHANG);
	}
	close(sock);
}

int main(int argc, char *argv[], char **envp)
{
	if(argc != 2)
	{
		printf("Please check your arguments.");
		return -1;
	}

	// Get Port number
	char *p;
	int port, errno = 0;
	long conv = strtol(argv[1], &p, 10);
	// printf("%d\n", conv);

	// Check for errors: e.g., the string does not represent an integer
	// or the integer is larger than int
	if (errno != 0 || *p != '\0' || conv <= INT_MIN) {
	    // Put here the handling of the error, like exiting the program with
	    // an error message
	    fprintf(stderr, "Invalid port number.\n");
	} else {
	    port = conv;    
	}

	listenOnPort(port);
}