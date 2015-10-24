// Distributed File System
// By: Josh Fermin
// 
// Followed:
// http://www.binarytides.com/server-client-example-c-sockets-linux/

#include <stdio.h>
#include <sys/socket.h>
#include <sys/errno.h> // for errexit
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
#include <dirent.h>
#include <stdarg.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <openssl/md5.h> // for md5

#include "configdfs.h"
#define MAX_BUFFER 2000

user *users;
user currUser;
char server_directory[256] = ".";
int server_port;
int num_users = 0;


void parseConfFile(const char *);
void processRequest(int);
void serverList(int, char * username);
void serverPut(int sock, char *);
int countLines(const char *);
int listenOnPort(int);
int authenticateUser(int, char *, char *);
void checkFileCurrServ(int, char * filename);
int requestFileCheck(char * filename);
int errexit(const char *format, ...);
void processRequest(int socket);

void parseConfFile(const char *filename)
{
    char * line = NULL;
    char * token;
    size_t len = 0;
    int read_len = 0;
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

void processRequest(int socket)
{
    char buf[MAX_BUFFER];
    char arg[64];
    char *token;
    char username[32], passwd[32];
    int read_size    = 0;
    int len = 0;
    char command[256];

    while ((read_size = recv(socket, &buf[len], (MAX_BUFFER-len), 0)) > 0)
    { 
        char line[read_size];
        strncpy(line, &buf[len], sizeof(line));
        len += read_size;
        line[read_size] = '\0';

        printf("Found:  %s\n", line);

        // printf("Line: %s\n", command);
        if (strncmp(line, "LOGIN:", 6) == 0)
        {
        	token = strtok(line, ": ");
	        token = strtok(NULL, " ");
            if (token == NULL){
                write(socket, "Invalid Command Format. Please try again.\n", 42);
                close(socket);
                return;
            }
	        strcpy(username, token);
            if (token == NULL){
                write(socket, "Invalid Command Format. Please try again.\n", 42);
                close(socket);
                return;
            }
	        token = strtok(NULL, " ");
	        strcpy(passwd, token);

    	}

        sscanf(line, "%s %s", command, arg);
        // printf("%s\n", command);
        // printf("%s\n", arg);

        // Check Command
        if (strncmp(command, "GET", 3) == 0) {
            printf("GET CALLED!\n");
            // getFile(command, connection);
        } else if(strncmp(command, "LIST", 4) == 0) {
            printf("LIST CALLED:\n");
        	send(socket, command , strlen(command), 0);
            // serverList(socket, username);
        } else if(strncmp(command, "PUT", 3) == 0){
            //Put Call
            printf("PUT Called!\n");
            serverPut(socket, arg);
        } else if(strncmp(command, "LOGIN", 4) == 0) {
                        //Check Username and Password
            if (authenticateUser(socket, username, passwd) == 0){
                char *message = "Invalid Username/Password. Please try again.\n";
                write(socket, message, strlen(message));
                close(socket);
                return;
            }
        } else {
            printf("Unsupported Command: %s\n", command);
        }
    }
    if(read_size == 0)
	{
        puts("Client disconnected");
        fflush(stdout);
	}
    else if(read_size == -1)
    {
        perror("recv failed");
    }
     
    return;
}

int authenticateUser(int socket, char * username, char * password) 
{
    int i;
    char directory[4096];

    strcpy(directory, server_directory);
    strcat(directory, username);
    for (i = 0; i < num_users; i++){
        if (strncmp(users[i].name, username, strlen(username)) == 0){
        	if(strncmp(users[i].password, password, strlen(password)) == 0)
        	{
	            currUser = users[i];
	            if(!opendir(directory)) {
	                write(socket, "Directory Doesn't Exist. Creating!\n", 35);
	                mkdir(directory, 0770);
	            }
	            printf("User Authenticated.\n");
	            return 1;
        	}
        }
    }
    return 0;
}




int listenOnPort(int port) 
{
	int socket_desc , client_sock, c, *new_sock;
    struct sockaddr_in server , client;
     
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( port );
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
     
    //Listen
    listen(socket_desc , 3);
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        puts("Connection accepted");
        new_sock = malloc(1);
        *new_sock = client_sock;
         
        if(fork() == 0){
	        printf("Connected! %d\n", server_port);         
	        processRequest(client_sock);    
	        exit(0);    	
        }
         
        puts("Handler assigned");
    }
     
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
    return 0;
}

void serverPut(int sock, char * arg)
{
    char buf[MAX_BUFFER];
    char file_loc[128];
    int file_size, remaining, len;
    FILE *file;
    char *auth = "Clear for transfer.";

    sprintf(file_loc, "%s%s/", server_directory, currUser.name);
    strncat(file_loc, arg, strlen(arg));
    printf("%s\n", file_loc);

    printf("Sending auth %s\n", file_loc);

    /* Send authentication reply */
    if (send(sock, auth , strlen(auth), 0) < 0)
        errexit("Failed to write: %s\n", strerror(errno));


    int rv;
    if ((rv = recv(sock, buf, MAX_BUFFER, 0)) < 0) 
        errexit("Failed to receive file size: %s\n", strerror(errno));
    file_size = atoi(buf);
    printf("we made it here1");

    if (!(file = fopen(file_loc, "w")))
        errexit("Failed to open file at: '%s' %s\n", file_loc, strerror(errno)); 
    printf("we made it here2");

    remaining = file_size;
    printf("we made it here3");
    while (((len = recv(sock, buf, MAX_BUFFER, 0)) > 0) && (remaining > 0)) {
        fwrite(buf, sizeof(char), len, file);
        remaining -= len;
        fprintf(stdout, "Received %d bytes\n", len);
    }

    fclose(file);
}
// void serverList(int sock, char * username){
//     printf("Getting Files for: %s\n", username);

//     DIR * d;
//     struct dirent *dir;
//     struct stat filedets;
//     int status;

//     char path[MAX_BUFFER];
//     char directory[4096];

//     strcpy(directory, server_directory);
//     strcat(directory, username);

//     d = opendir(directory);

//     if (d) {
//         while ((dir = readdir(d)) != NULL) {
//             if (strcmp(".", dir->d_name) == 0){
//                 //Skip Current Dir
//             } else if (strcmp("..", dir->d_name) == 0){
//                 //Skip Prev Dir
//             } else {
//                 sprintf(path, "%s/%s", directory, dir->d_name);
//                 status = lstat(path, &filedets);
//                 if(S_ISDIR(filedets.st_mode)) {
//                     //Skip Directories
//                 } else {
//                     printf("%s\n", dir->d_name);
//                     checkFileCurrServ(sock, dir->d_name);
//                     //TODO: Check for Pieces on Other Servers
//                 }
//             }
//         }

//         closedir(d);
//     } 
// }

// //Check the Current Server for the File
// //Request Check on other Server if not found
// void checkFileCurrServ(int sock, char * filename){

//     // printf("File: %s\n", filename);
    
//     char ext[8] = "";
//     char filenopart[256] = "";
//     char currpart[256] = "";
//     char path[256] = "";

//     int fd;

//     strcpy(filenopart, strtok (filename, "."));

//     strcpy(ext, strtok (NULL, "."));

//     if (strtok (NULL, ".") != NULL){
//         sprintf(filenopart, "%s.%s", filenopart, ext);
//     } 

//     //TODO Check to See if we Already Saw that File

//     // printf("Filename: %s\n",filenopart);

//     int part1 = 0;
//     int part2 = 0;
//     int part3 = 0;
//     int part4 = 0;

//     //Check Current Server for Part
//     sprintf(path, "%s%s/", server_directory, currUser.name);

//     strcpy(currpart, filenopart);
//     strcat(currpart, ".1");
    
//     strcat(path, currpart);

//     // printf("Path: %s FD: %d\n", path, access( path, F_OK ));

//     if ((fd = open(path, O_RDONLY)) != -1){
//         part1 = 1;  
//         // printf("CWe found part1\n");

//     } else {
//         if (requestFileCheck(currpart)){
//             // printf("SWe found part1\n");
//             part1 = 1;
//         }
//     }
//     close(fd);

//     sprintf(path, "%s%s/", server_directory, currUser.name);

//     strcpy(currpart, filenopart);
//     strcat(currpart, ".2");
    
//     strcat(path, currpart);

//     if ((fd = open(path, O_RDONLY)) != -1){
//         part2 = 1;  
//         // printf("CWe found part3\n");
//     } else {
//          if (requestFileCheck(currpart)){
//             part2 = 1;
//             // printf("SWe found part3\n");
//         }
//     }
//     close(fd);


//     sprintf(path, "%s%s/", server_directory, currUser.name);

//     strcpy(currpart, filenopart);
//     strcat(currpart, ".3");
    
//     strcat(path, currpart);

//     if ((fd = open(path, O_RDONLY) != -1)){
//         part3 = 1;  
//         // printf("CWe found part3\n");

//     } else {
//          if (requestFileCheck(currpart)){
//             part3 = 1;
//             // printf("SWe found part3\n");

//         }
//     }
//     close(fd);

//     sprintf(path, "%s%s/", server_directory, currUser.name);

//     strcpy(currpart, filenopart);
//     strcat(currpart, ".4");
    
//     strcat(path, currpart);

//     if ((fd = open(path, O_RDONLY) != -1)){
//         part4 = 1;  
//         // printf("CWe found part4\n");

//     } else {
//          if (requestFileCheck(currpart)){
//             part4 = 1;
//             // printf("SWe found part4\n");
//         }
//     }
//     close(fd);


//     //Add File to Check List

//     if ((part1+part2+part3+part4) == 4){
//         write(sock, filenopart, strlen(filenopart));
//         write(sock, "\n", 1);
//     } else {
//         write(sock, filenopart, strlen(filenopart));
//         write(sock, " [incomplete]", 13);
//         write(sock, "\n", 1);
//     }
// }


// //Request a Check on another Server
// int requestFileCheck(char * filename){

//     //Connect to Each Servers
//     static int currport = 10001;
//     char res[2];
//     int servfd = 0;

//     char command[4096];

//     sprintf(command, "CHECK %s %s %s\n", currUser.name, currUser.password, filename);

//     // printf("Command RFC: %s\n", command);

//     for(currport = 10001; currport<10005; currport++){

//         // printf("Port: %d\n", currport);    

//         if (currport == server_port){

//         } else {
//             //Connect to Other Server
//             // printf("Trying to connect to server: %d\n", currport);

//             servfd = open_clientfd("localhost", currport);
            
//             //TODO 1 sec timeout

//             write(servfd, command, strlen(command));
//             readline(servfd, res, 2);

//             // printf("We read: %s %d\n", res, strncmp(res, "1", 1));

//             if(strncmp(res, "1", 1) == 0){
//                 // printf("WE FOUND IT!\n");
//                 return 1;
//             }
//             close(servfd);
//         }
//     }

//     return 0;
// }

int errexit(const char *format, ...) {
        va_list args;

        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        exit(1);
}

int main(int argc, char *argv[], char **envp)
{
	if(argc != 3)
	{
		printf("Please check your arguments.");
		return -1;
	}

	// create a folder which will hold the contents of the server
    strcat(server_directory, argv[1]);
    strcat(server_directory, "/");
    if(!opendir(server_directory))
    {
    	mkdir(server_directory, 0770);    	
    }

    server_port = atoi(argv[2]);

    parseConfFile("server/dfs.conf");

    listenOnPort(server_port);

    return 1;
}