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
#include <net/if.h>
#include <netdb.h>
#include <time.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define INT_MIN 1024
#define MAX_BUFFER 2000

int listenOnPort();
void accept_request(int);
void set_up_dnat(struct sockaddr_in);
void remove_http(char*);
void *get_in_addr(struct sockaddr *);

void accept_request(int sock)
{
	char buf[MAX_BUFFER];
	int read_size = 0, len = 0;
    int server;

	struct sockaddr_in s_hints;
    struct sockaddr_in server_addr;
    struct sockaddr_in p_addr;
    struct sockaddr_in d_addr;
	socklen_t s_addrlen; 
	socklen_t p_addrlen; 
	socklen_t d_addrlen; 
	s_addrlen = sizeof(struct sockaddr_in);
	p_addrlen = sizeof(struct sockaddr_in);			
	d_addrlen = sizeof(struct sockaddr_in);

	if ((server = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("\nserver: socket");
		exit(-1);
	}

	s_hints.sin_family = AF_INET;
	s_hints.sin_port = 0;
	s_hints.sin_addr.s_addr = INADDR_ANY; 
	bzero(&s_hints.sin_zero, 8);

	if((bind(server, (struct sockaddr *) &s_hints, sizeof(s_hints))) == -1)
	{
		perror("bind: ");
		exit(-1);
	}

	if((getsockopt(sock, SOL_IP, 80, (struct sockaddr *) &d_addr, &d_addrlen)) == -1){
		perror("getsockopt: ");
		exit(-1);
	}

	if((getsockname(server, (struct sockaddr *) &server_addr, &s_addrlen)) == -1){
		perror("getsockname: ");
		exit(-1);		
	}

	if((getpeername(sock, (struct sockaddr *) &p_addr, &p_addrlen)) == -1){
		perror("getpeername: ");
		exit(-1);		
	}


	char Snat[MAX_BUFFER];
	sprintf(Snat, "iptables -t nat -A POSTROUTING -p tcp -j SNAT --sport %d --to-source %s", ntohs(server_addr.sin_port), inet_ntoa(p_addr.sin_addr));
	printf("%s\n", Snat);	
	system(Snat);

	// int server_fd;
 //    //Finally, connect to the server
 //  	server_fd = connect(server, (struct sockaddr *) &d_addr, d_addrlen);
 //    if (server_fd == -1)
 //    	exit(1);



  	char *logfile;
    char logs[MAX_BUFFER];
    time_t currentTime;
    currentTime = time(NULL);
    //Getting the date and time
	logfile = ctime(&currentTime);
	//Replacing the new line character
	logfile[strlen(logfile)-1] = ' ';
	//Add the client's IP address to the logfile
	strcat(logfile, inet_ntoa(p_addr.sin_addr));
	//Convert the client's port number to a string
	sprintf(logs, " %d ", ntohs(p_addr.sin_port));
	strcat(logfile, logs);
	//Add the server's IP address to the logfile
	strcat(logfile, inet_ntoa(d_addr.sin_addr));
	memset(logs, 0, MAX_BUFFER);
	//Convert the server's port number to a string
	sprintf(logs, " %d ", ntohs(d_addr.sin_port));
	strcat(logfile, logs);

	FILE * FileLog = fopen("logging.txt", "a+");
    fprintf(FileLog, "%s", logfile);
    fclose(FileLog);

	while ((read_size = recv(sock, &buf[len], (MAX_BUFFER-len), 0)) > 0)
	{ 
		char line[read_size];
		strncpy(line, &buf[len], sizeof(line));
		len += read_size;
		line[read_size] = '\0';

		printf("Found:  %s\n", line);
		send(server_fd, line, strlen(line), 0);
	}


}

void set_up_dnat(struct sockaddr_in svr_addr)
{
	int fd;
	struct ifreq ifr;
	char Dnat[MAX_BUFFER];

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, "eth1", IFNAMSIZ-1);
	ioctl(fd, SIOCGIFADDR, &ifr);
	close(fd);

    sprintf(Dnat, "iptables -t nat -A PREROUTING -p tcp -i eth1 -j DNAT --to %s:%d", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), ntohs(svr_addr.sin_port));
    printf("calling iptables dnat: %s\n", Dnat);
    system(Dnat);
    printf("\nserver port %d: waiting for connections...\n", ntohs(svr_addr.sin_port));
}

int listenOnPort()
{
	int port = 8080;
	int client_fd, status;
	char s[INET6_ADDRSTRLEN];
	socklen_t sin_size;

	// sockaddr: structure to contain an internet address.
	struct sockaddr_in svr_addr, cli_addr;
	socklen_t sin_len = sizeof(cli_addr);
	struct sockaddr_storage their_addr;

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
		printf("listening with port %d\n", port);

	set_up_dnat(svr_addr);

	while (1)
	{
		sin_size = sizeof their_addr;

        // if connection
        inet_ntop(their_addr.ss_family, 
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server port %d: got connection from %s\n", port, s);

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

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in*)sa)->sin_addr);

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main()
{
	listenOnPort();
	return 1;
}