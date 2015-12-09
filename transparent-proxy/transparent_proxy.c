#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <dirent.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <time.h>

#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define INT_MIN 1024
#define MAX_BUFFER 2000

int listenOnPort();
void accept_request(int);
void remove_http(char*);

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

void remove_http(char* host)
{
	if(strncmp(host, "http://", 7) == 0)
	{
		memmove(host, host + 7, (512 - 7) / sizeof(host[0])); // remove http://
	}

	if(strncmp(host, "https://", 8) == 0)
	{
		memmove(host, host + 8, (512 - 8) / sizeof(host[0])); // remove https://
	}
}

int listenOnPort()
{
	int port = 8080;
	int client_fd, status;
	char DnatRules[MAX_BUFFER];

	// sockaddr: structure to contain an internet address.
	struct sockaddr_in svr_addr, cli_addr;
	socklen_t sin_len = sizeof(cli_addr);

	// create endpoint for comm, AF_INET (IPv4 Internet Protocol), 
	// SOCK_STREAM (Provides sequenced, reliable, two-way, connection-based byte streams),
	int sock = socket(AF_INET, SOCK_STREAM, 0); // returns file descriptor for new socket
	if (sock < 0)
		fprintf(stderr, "Can't open socket\n");

	svr_addr.sin_family = AF_INET;
	svr_addr.sin_addr.s_addr = INADDR_ANY;
	svr_addr.sin_port = htons(port); // host to network short (htons)
	bzero(svr_addr.sin_zero, 8);
	// Start server
		if (bind(sock, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) == -1) {
			close(sock);
			fprintf(stderr, "Can't bind socket.\n");
			// err(1, "Can't bind");
		}
	 
		listen(sock, 5); // 5 is backlog - limits amount of connections in socket listen queue
		printf("listening on localhost with port %d\n", port);

	int fd;
	struct ifreq ifr;
	//This grabs the information from ifconfig for eth1, in order to write the DNAT rule
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, "eth1", IFNAMSIZ-1);
	ioctl(fd, SIOCGIFADDR, &ifr);
	close(fd);

	//Writing out the DNAT rule
    sprintf(DnatRules, "iptables -t nat -A PREROUTING -p tcp -i eth1 -j DNAT --to %s:%d", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), ntohs(svr_addr.sin_port));
    printf("DNAT RULES: %s\n", DnatRules);
    system(DnatRules);
    printf("\nserver port %d: waiting for connections...\n", ntohs(svr_addr.sin_port));

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

int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		printf("Please check your arguments.");
		return -1;
	}


	listenOnPort();
}
//http://cboard.cprogramming.com/c-programming/142841-sending-http-get-request-c.html