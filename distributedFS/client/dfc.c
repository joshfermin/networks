// Distributed File System
// By: Josh Fermin
// 
// Followed:
// http://www.binarytides.com/server-client-example-c-sockets-linux/

#include <stdio.h>
#include <sys/errno.h> // for errexit
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
#include <stdarg.h>
#include <fcntl.h> //(for O_*)
#include <openssl/md5.h> // for md5

#include "configdfc.h"
#define MAX_BUFFER 2000

int parseConfFile(const char *);
void readUserInput();
int connectSocket(int, const char *);
void list(char *);
int put(char *);
int get(char *);
void authenticateUser(int, char *, char *);
int errexit(const char *format, ...);
void recieveReplyFromServer(int sock);
int attemptToConnect();

const char *FILE_DIR="./files";
char username[128];
char password[128];
server * servers; 
int num_servers;

// Parses a file you give it
int parseConfFile(const char *filename)
{
	char *line;
    char * token;
    char head[64], tail[256], tail2[256];
    size_t len = 0;
    int read_len = 0;
    int i=0;
    FILE* conf_file = fopen(filename, "r");
    if (conf_file == NULL)
        fprintf(stderr, "Could not open config file.\n");
    servers = malloc(4 * sizeof(server));

    while((read_len = getline(&line, &len, conf_file)) != -1) {
        line[read_len+1] = '\0';
        if (line[0] == '#')
            continue;

        sscanf(line, "%s %s %s", head, tail, tail2);
        if (!strcmp(head, "Server")) {
            token = strtok(tail2, ":");
            strncpy(servers[i].host,token, 20);
            token = strtok(NULL, " ");
            servers[i].port= atoi(token);
            i++;
        } 
        if (!strcmp(head, "Username:")) {
            sscanf(tail, "%s", username);
            // confdfc.username[strlen(confdfc.username)-1] = '\0';
        } 
        if (!strcmp(head, "Password:")) {
            sscanf(tail, "%s", password);
            // confdfc.password[strlen(confdfc.password)-1] = '\0';
        }
    }

    // printf("%d\n", i);
    // printf("%s\n", password);
    // printf("%s\n", servers[0].host);
    fclose(conf_file);

    return i;
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
            // printf("the command you entered was: %s\n", command);
            // printf("sock num %d\n", sock);
            list(command);
        }
        else if (!strncasecmp(command, "GET", 3)) {
            if (strlen(line) <= 3)
                printf("Check your args.\n");
            else
                get(line);
        }
        else if (!strncasecmp(command, "PUT", 3)) {
            if (strlen(line) <= 3)
                printf("Check your args.\n");
            else
                put(line);
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

void recieveReplyFromServer(int sock){
    char server_reply[MAX_BUFFER];
    if( recv(sock, server_reply, 2000, 0) < 0)
    {
        errexit("Error in recv: %s\n", strerror(errno));
        // puts("recv failed");
    } else {
        // server_reply[len] = '\0'; // null terminate server_reply
        puts(server_reply);
    }
}

void authenticateUser(int sock, char * username, char * password)
{
    char *result = malloc(strlen(username)+strlen(password));//+1 for the zero-terminator
    // char buf[MAX_BUFFER];
    // int len;

    strcpy(result, "LOGIN:");
    strcat(result, username);
    strcat(result, " ");
    strcat(result, password);
    if (write(sock, result, strlen(result)) < 0){
        // puts("Authentication failed");
        errexit("Error in Authentication: %s\n", strerror(errno));
    }
    recieveReplyFromServer(sock);
}

void list(char *command)
{

    int sock = attemptToConnect();
    if(write(sock, command, strlen(command)) < 0) {
        // puts("List failed");
        errexit("Error in List: %s\n", strerror(errno));
    }
    recieveReplyFromServer(sock);
}

int attemptToConnect()
{
    int sock;
    int i;
    // printf("numservers %d\n",num_servers );
    for (i = 0; i < num_servers; ++i)
    {
        servers[i].fd = connectSocket(servers[i].port, servers[i].host);
        if(servers[i].fd != 1)
        {
            sock = servers[i].fd;
            // printf("%d\n", sock);
            return sock;
            // break; // found connection
        }
    }
    return 0;
}

// send file via socket
// http://stackoverflow.com/questions/2014033/send-and-receive-a-file-in-socket-programming-in-linux-with-c-c-gcc-g
int put(char *line)
{
    char file_loc[128];
    int fd;
    char file_size[256];
    char req[128];
    int size;
    struct stat file_stat;
    char command[8], arg[64];
    char buffer[MAX_BUFFER];
    MD5_CTX mdContext;
    char md5buf[MAX_BUFFER];
    // int sock = attemptToConnect();
    int rv, i, count = num_servers;
    char byte[64];
    char sum[MD5_DIGEST_LENGTH];
    int order;
    int order_mat[num_servers][2];

    struct timespec tim;
    tim.tv_sec = 0;
    tim.tv_nsec = 100000000L; /* 0.1 seconds */

    sscanf(line, "%s %s", command, arg);
    sprintf(file_loc, "%s/%s", FILE_DIR, arg);


    for (i = 0; i < num_servers; ++i)
    {
        servers[i].fd = connectSocket(servers[i].port, servers[i].host);
    }

 

    if ((fd = open(file_loc, O_RDONLY)) < 0)
        errexit("Failed to open file at: '%s' %s\n", file_loc, strerror(errno)); 

    if (fstat(fd, &file_stat) < 0)
        errexit("Error fstat file at: '%s' %s\n", file_loc, strerror(errno));

    size = file_stat.st_size;
    sprintf(file_size, "%d", size);

    /* Get MD5sum of file to select server order */
    MD5_Init(&mdContext);
    while ((rv = read(fd, &md5buf, MAX_BUFFER)) != 0)
        MD5_Update(&mdContext, md5buf, rv);
    MD5_Final(sum, &mdContext);
     // Use modulus on first byte of MD5 hash 
    sprintf(byte, "%02x", sum[0]);
    order = (strtol(byte, NULL, 16) % count);

    close(fd);

    /* Calculate order matrix
     * (4 X 2) Piece number X Server number
     */
     for (i = 1; i < count + 1; i++) {
        order_mat[(i % count)][0] = i-1;
        order_mat[(i % count)][1] = (i % count);
     }
    printf("Size: %d Using: %ld\n", size, size / count);
    sprintf(file_size, "%ld", size / count);
    printf("%s\n", file_size);

    /* Format new filename */
    sprintf(req, "PUT %s.", arg);
    // printf("%s\n", req);

    int offset = 0;
    char piece_name[128];
    // printf("%d\n", count);
    // count = 4;
    for (i = 0; i < count; i++) {

        sprintf(piece_name, "%s%d", req, i+1);
        printf("%s\n", piece_name); 

        if (write(servers[order_mat[i][0]].fd, piece_name, strlen(piece_name)) < 0){
            errexit("Error in List: %s\n", strerror(errno));
        } 

        // nanosleep(&tim, NULL); 

        nanosleep(&tim, NULL); 

        if(write(servers[order_mat[i][0]].fd, file_size, strlen(file_size)) < 0) {
            // puts("List failed");
            errexit("Error in List: %s\n", strerror(errno));
        }
        // printf("0: %d\n", order_mat[i][0]);
        // printf("1: %d\n", order_mat[i][1]);

        nanosleep(&tim, NULL); 

        while (1) {
            // Read data into buffer.  We may not have enough to fill up buffer, so we
            // store how many bytes were actually read in bytes_read.
            int bytes_read = read(fd, buffer, sizeof(buffer));
            if (bytes_read == 0) // We're done reading from the file
                break;

            if (bytes_read < 0) {
                // handle errors
            }

            // You need a loop for the write, because not all of the data may be written
            // in one call; write will return how many bytes were written. p keeps
            // track of where in the buffer we are, while we decrement bytes_read
            // to keep track of how many bytes are left to write.
            void *p = buffer;
            while (bytes_read > 0) {
                int bytes_written = write(servers[order_mat[i][0]].fd, p, bytes_read);
                if (bytes_written <= 0) {
                    // handle errors
                }
                bytes_read -= bytes_written;
                p += bytes_written;
            }
            // while (bytes_read > 0) {
            //     int bytes_written = write(servers[i][1], p, bytes_read);
            //     if (bytes_written <= 0) {
            //         // handle errors
            //     }
            //     bytes_read -= bytes_written;
            //     p += bytes_written;
            // }
        }
        offset += (size/count);
    }

    puts("done with forloop");
    // if (write(sock, file_size, sizeof(file_size)) < 0)
    //     errexit("Echo write: %s\n", strerror(errno));
    return 0;  
}

int get(char *line)
{
    char buf[MAX_BUFFER];
    int read_size    = 0;
    int len = 0;
    FILE *downloaded_file;
    char command[8], arg[64];
    char file_loc[128];
    int sock = attemptToConnect();

    sscanf(line, "%s %s", command, arg);
    sprintf(file_loc, "./%s", arg);

    if(write(sock, line, strlen(line)) < 0) {
        // puts("List failed");
        errexit("Error in List: %s\n", strerror(errno));
    }

    downloaded_file = fopen(file_loc, "w");
    if (downloaded_file == NULL)
    {
        printf("Error opening file!\n");
        exit(1);
    }


    while ((read_size = recv(sock, &buf[len], (MAX_BUFFER-len), 0)) > 0)
    { 
        char line[read_size];
        strncpy(line, &buf[len], sizeof(line));
        len += read_size;
        line[read_size] = '\0';

        printf("%s\n", line);
        fprintf(downloaded_file, "%s\n", line);
        fclose(downloaded_file);
        return 1;
    }

    return 0;
}

// Connect to a certain socket
int connectSocket(int port, const char *hostname)
{
    int sock;
    struct sockaddr_in server;
     
    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , IPPROTO_TCP);
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    // puts("Socket created");
     // printf("port num %d\n", port);
    server.sin_addr.s_addr = inet_addr(hostname);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
 
    //Connect to remote server
    if (connect(sock, (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        // perror("connect failed. Error");
        return 1;
    }

    authenticateUser(sock, username, password);

    // printf("Socket %d connected on port %d\n", sock, ntohs(server.sin_port));

    return sock;
}


int errexit(const char *format, ...) {
        va_list args;

        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        exit(1);
}

int main(int argc, char *argv[], char **envp)
{
    if(argv[1])
    {
        // int server_fd[64];

        // Get the number of servers
        num_servers = parseConfFile(argv[1]);

        // Try to connect to the following servers.
        printf("There are %d servers in the config file.\n", num_servers);

        readUserInput();
        return 1;
    }
    else
    {
        printf("Please specify a config file.\n");
    }


}