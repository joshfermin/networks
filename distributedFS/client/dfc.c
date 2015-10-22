#include <stdio.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>

#include "configdfc.h"

void parseConfFile(const char *);
void readUserInput();
int connectSocket(const char *port, const char *hostname);
void list();
int put(char *filename);
int get(char *filename);

char username[128];
char password[128];
server * servers; 


// Parses a file you give it
void parseConfFile(const char *filename)
{
	char *line;
    char * token;
    char head[64], tail[256], tail2[256];
    size_t len = 0;
    int read_len = 0;
    int i=0;
    FILE* conf_file = fopen(filename, "r");
    if (conf_file == NULL)
        exit(EXIT_FAILURE);

    servers = malloc(4 * sizeof(servers));

    while((read_len = getline(&line, &len, conf_file)) != -1) {
        // printf("%s",line);
        line[read_len-1] = '\0';
        if (line[0] == '#')
            continue;

        sscanf(line, "%s %s %s", head, tail, tail2);
        if (!strcmp(head, "Server")) {
            token = strtok(tail2, ":");
            // printf("Host: %s\n", token);
            strcpy(servers[i].host,token);

            token = strtok(NULL, " ");
            // printf("Port: %s\n", token);
            servers[i].port= atoi(token);
        } 
        if (!strcmp(head, "Username:")) {
            sscanf(tail, "%s", username);
            // confdfc.username[strlen(confdfc.username)-1] = '\0';
        } 
        if (!strcmp(head, "Password:")) {
            sscanf(tail, "%s", password);
            // confdfc.password[strlen(confdfc.password)-1] = '\0';
        }
        i++;
    }
    // printf("%s\n", username);
    // printf("%s\n", servers[0].host);
    fclose(conf_file);
}

// Reads user input to get LIST, PUT, GET, QUIT, HELP commands
void readUserInput() {
    char *line = NULL;      /* Line read from STDIN */
    size_t len;
    ssize_t read;
    char command[8], arg[64];
    int status = 1;


    while (status) {
        printf("%s> ", username);
        read = getline(&line, &len, stdin);
        line[read-1] = '\0';

        sscanf(line, "%s %s", command, arg);
        if (!strncasecmp(command, "LIST", 4)) {
            printf("the command you entered was: %s\n", command);

        }
        else if (!strncasecmp(command, "GET", 3)) {

        }
        else if (!strncasecmp(command, "PUT", 3)) {

        }
        else if (!strncasecmp(command, "QUIT", 4) || !strncasecmp(command, "EXIT", 4)) {
            status = 0;
        }
        else if (!strncasecmp(command, "HELP", 4)) {
            printf("You can enter the following commands: LIST, GET, PUT, EXIT and QUIT\n");
        }
        else{
            printf("Invalid command. Type \"HELP\" for more options.\n");
        }
    }
    printf("Shutting down...\n");
}

void list()
{

}

int put(char *filename)
{

    return 0;
}

int get(char *filename)
{

    return 0;
}

// Connect to a certain socket
int connectSocket(const char *port, const char *hostname)
{
    struct hostent  *phe;     
    struct sockaddr_in sockin;
    int sock;                 

    memset(&sockin, 0, sizeof(sockin));

    sockin.sin_family = AF_INET;

    /* Convert and set port */
    sockin.sin_port = htons((unsigned short)atoi(port));
    if (sockin.sin_port == 0)
        fprintf(stderr, "Error getting port: %s\n", port);

    /* Map host name to IP address, allowing for dotted decimal */
    if ((phe = gethostbyname(hostname)))
        memcpy(&sockin.sin_addr, phe->h_addr, phe->h_length);
    else if ( (sockin.sin_addr.s_addr = inet_addr(hostname)) == INADDR_NONE )
        fprintf(stderr, "Cant get host %s\n", hostname);

    // Create a socket
    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0)
        fprintf(stderr, "Cant create socket %s\n", strerror(errno));

    /* Connect the socket */
    if (connect(sock, (struct sockaddr *)&sockin, sizeof(sockin)) < 0)
        return -1; /* Connection failed */

    printf("Socket %d connected on port %d\n", sock, ntohs(sockin.sin_port));
    return sock;
}


int main(int argc, char *argv[], char **envp)
{
    if(argv[1])
    {
        parseConfFile(argv[1]);
        readUserInput();
    }
    else
    {
        printf("Please specify a config file.\n");
    }
}