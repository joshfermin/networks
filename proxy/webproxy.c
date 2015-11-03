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
#include <netdb.h> // for struct hostent

#define INT_MIN 1024
#define MAX_BUFFER 2000

int listenOnPort(int);
void accept_request(int);
void process_get(char*, int, char*);
void remove_http(char*);

void accept_request(int socket)
{
	char buf[MAX_BUFFER];
	int read_size = 0, len = 0;

	while ((read_size = recv(socket, &buf[len], (MAX_BUFFER-len), 0)) > 0)
	{ 
		char line[read_size];
		char method[32];
		char host[512];
		char http_version[64];
		strncpy(line, &buf[len], sizeof(line));
		len += read_size;
		line[read_size] = '\0';

		printf("Found:  %s\n", line);

		sscanf(line, "%s %s %s", method, host, http_version);
		// printf("%s %s %s\n", method, host, http_version);

		
		if(strncmp(method, "GET", 3) == 0)
		{
			printf("Processing GET");
			process_get(host, socket, line);
		} else {
			printf("This is not a supported method\n");
		}
		

	    // else
	    // {
	    //     printf("\n%s = ", server->h_name);
	    //     unsigned int j = 0;
	    //     while (server -> h_addr_list[j] != NULL)
	    //     {
	    //         printf("%s", inet_ntoa(*(struct in_addr*)(server -> h_addr_list[j])));
	    //         j++;
	    //     }
	    // }


    	// error = getaddrinfo(host, NULL, NULL, &result);
    	// if (error != 0)
	    // {   
	    //     if (error == EAI_SYSTEM)
	    //     {
	    //         perror("getaddrinfo");
	    //     }
	    //     else
	    //     {
	    //         fprintf(stderr, "error in getaddrinfo: %s\n", gai_strerror(error));
	    //     }   
	    //     exit(EXIT_FAILURE);
	    // }   
	    // for (res = result; res != NULL; res = res->ai_next)
	    // {   
	    //     char hostname[1025];

	    //     error = getnameinfo(res->ai_addr, res->ai_addrlen, hostname, 1025, NULL, 0, 0); 
	    //     if (error != 0)
	    //     {
	    //         fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(error));
	    //         continue;
	    //     }
	    //     if (*hostname != '\0')
	    //         printf("hostname: %s\n", hostname);
	    // }   

	    // freeaddrinfo(result);

	}
}

void process_get(char* host, int sock, char* request)
{
	int counter = 0;
	int error, i;
	char firstHalf[500];
    char secondHalf[500];
    struct hostent *server;
    struct sockaddr_in serveraddr;

	remove_http(host);

	// not being used right now...
	for (i = 0; i < strlen(host); i++)
    {
        if (host[i] == '/')
        {
                strncpy(firstHalf, host, i);
                firstHalf[i] = '\0';
                break;
        }     
    }
    
    for (i; i < strlen(host); i++)
    {
        strcat(secondHalf, &host[i]);
        break;
    }

    if(host[strlen(host) - 1] == '/')
    {
    	host[strlen(host)-1] = '\0';
    }
    // removeChar(secondHalf, '\\');
    
    printf("firsthalf: %s second: %s host: %s\n", firstHalf, secondHalf, host );

	server = gethostbyname(host);

	if (server == NULL)
    {
    	// write(sock, result, strlen(result));
        printf("gethostbyname() failed\n");
    }

    printf("Official name is: %s\n", server->h_name);
	printf("IP address: %s\n", inet_ntoa(*(struct in_addr*)server->h_addr));

    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;

    bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length);
     
    serveraddr.sin_port = htons(80);
}

void remove_http(char* host)
{
	if(strncmp(host, "http://", 7) == 0)
	{
		printf("remove http\n");
		memmove(host, host + 7, (512 - 7) / sizeof(host[0])); // remove http://
	}

	if(strncmp(host, "https://", 8) == 0)
	{
		printf("remove https\n");
		memmove(host, host + 8, (512 - 8) / sizeof(host[0])); // remove https://
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

//https://github.com/shawnzam/webproxy-assignment-/blob/master/main.c
//http://cboard.cprogramming.com/c-programming/142841-sending-http-get-request-c.html