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
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>

#include "configdfs.h"
user *users;
char server_directory[256] = ".";
int server_port;

void parseConfFile(const char *);
void processRequest(int);
int countLines(const char *);
int listenOnPort(int);
int readLine(int, char *, int);

void parseConfFile(const char *filename)
{
    char * line = NULL;
    char * token;
    size_t len = 0;
    int read_len = 0;
    int num_users = 0;
    int i=0;

    // open config file for reading
    FILE *conf_file = fopen(filename, "r");
    if (conf_file == NULL)
        fprintf(stderr, "Could not open file.");

    // get number of users
    num_users = countLines(filename);
    // printf("%d\n", num_users);
    users = malloc(num_users * sizeof(user));

    // for each line in config, add new user struct
    while((read_len = getline(&line, &len, conf_file)) != -1) {
        token = strtok(line, " ");
        strcpy(users[i].name, token);

        token = strtok(NULL, " ");
        strcpy(users[i].password,token);

        // printf("Name: %s Password: %s\n",users[i].name, users[i].password);
        i++;
    }
    fclose(conf_file);
}

int countLines(const char *filename)
{
	FILE *fp = fopen(filename,"r");
	int ch=0;
	int lines=1;

	while(!feof(fp))
	{
		ch = fgetc(fp);
		if(ch == '\n')
		{
		lines++;
		}
	}
	return lines;
}


int readLine(int fd, char * buf, int maxlen)
{
    int nc, n = 0;
    for(n=0; n < maxlen-1; n++)
    {
        nc = read(fd, &buf[n], 1);
        if( nc <= 0) return nc;
        if(buf[n] == '\n') break;
    }
    buf[n+1] = 0;
    return n+1;
}

void processRequest(int connection)
{
    char command[4096] = ""; 
    char * token;
    char username[256];
    char passwd[256];

    while(1) {

        //Read Command, Host and Keep-Alive
        readLine(connection, command, 4096);

        printf("Line: %s\n", command);

        //Grab UserName and Password
        token = strtok(command, " ");

        token = strtok(NULL, " ");
        if (token == NULL){
            write(connection, "Invalid Command Format. Please try again.\n", 42);
            close(connection);
            return;
        }
        strcpy(username, token);

        token = strtok(NULL, " ");
        if (token == NULL){
            write(connection, "Invalid Command Format. Please try again.\n", 42);
            close(connection);
            return;
        }
        strcpy(passwd, token);

        token = strtok(NULL, " ");

        //Check Username and Password
        // if (checkUser(username, passwd) == 0){
        //     write(connection, "Invalid Username/Password. Please try again.\n", 45);
        //     close(connection);
        //     return;
        // } 

        // Check Command
        if (strncmp(command, "GET", 3) == 0) {
            //Token File Name
            printf("GET CALLED!\n");
            // getFile(command, connection);
        } else if(strncmp(command, "LIST", 4) == 0) {
            //List Call
            printf("LIST Called!\n");
            // listFiles(username);
        } else if(strncmp(command, "PUT", 3) == 0){
            //Put Call
            printf("PUT Called!\n");

        } else if(strncmp(command, "CHECK", 5) == 0){
            //Put Call
            // checkServer(token, username, passwd);
            printf("CHECK Called!\n");

        }else {
            printf("Unsupported Command: %s\n", command);
            close(connection);
            return;

        }
    }
}

int listenOnPort(int port) 
{
    int sock, optval=1;
    struct sockaddr_in serveraddr;

    // Create a socket descriptor 
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    // Eliminates "Address already in use" error from bind. 
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, 
                (const void *)&optval , sizeof(int)) < 0)
        return -1;


    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serveraddr.sin_port = htons((unsigned short)port); 
    if (bind(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    // Make it a listening socket ready to accept connection requests 
    if (listen(sock, 1024) < 0)
        return -1;
    return sock;
}

int main(int argc, char *argv[], char **envp)
{
	int connection;
	if(argc != 3)
	{
		printf("Please check your arguments.");
		return -1;
	}

	// create a folder which will hold the contents of the server
    strcat(server_directory, argv[1]);
    strcat(server_directory, "/");
    mkdir(server_directory, 0770);
    server_port = atoi(argv[2]);

    parseConfFile("server/dfs.conf");

    int sock;
    socklen_t client_len = sizeof(struct sockaddr_in);
    struct sockaddr_in client_addr;

    sock = listenOnPort(server_port);


    // 
    while (1) {
        connection = accept(sock, (struct sockaddr*)&client_addr, &client_len);
        printf("Connected! %d\n", server_port );         
        processRequest(connection);
    }

    return 1;
}