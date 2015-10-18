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

#include "configdfc.h"

void parseConfFile(const char *);
void read_user_input();

void parseConfFile(const char *filename)
{
	char *line;
    char head[64], tail[256], tail2[256];
    size_t len = 0;
    int read_len = 0;

    FILE* conf_file = fopen(filename, "r");
    while((read_len = getline(&line, &len, conf_file)) != -1) {
        // printf("%s",line);
        line[read_len-1] = '\0';
        if (line[0] == '#')
            continue;
        sscanf(line, "%s %s %s", head, tail, tail2);
        if (!strcmp(head, "Server")) {
            if(!strcmp(tail, "DFS1"))
            {
                sscanf(tail2, "%s", confdfc.DFS1);
            }
            if(!strcmp(tail, "DFS2"))
            {
                sscanf(tail2, "%s", confdfc.DFS2);
            }
            if(!strcmp(tail, "DFS3"))
            {
                sscanf(tail2, "%s", confdfc.DFS3);
            }
            if(!strcmp(tail, "DFS4"))
            {
                sscanf(tail2, "%s", confdfc.DFS4);
            }
        } 
        if (!strcmp(head, "Username:")) {
            sscanf(tail, "%s", confdfc.username);
            // confdfc.username[strlen(confdfc.username)-1] = '\0';
        } 
        if (!strcmp(head, "Password:")) {
            sscanf(tail, "%s", confdfc.password);
            // confdfc.password[strlen(confdfc.password)-1] = '\0';
        }
    }
    fclose(conf_file);
}

void read_user_input() {
    char *line = NULL;      /* Line read from STDIN */
    char *token;
    ssize_t len;
    char command[8], arg[64];
    int status = 1;

    // printf("Servers: %d\n", config.server_count);

    while (status) {
        printf("%s> ", confdfc.username);
        getline(&line, &len, stdin);

        sscanf(line, "%s %s", command, arg);
        if (!strncasecmp(command, "LIST", 4)) {
            // send_request(0, line);
            printf("the command you entered was: %s\n", command);

        }
        if (!strncasecmp(command, "GET", 3)) {

        }
        if (!strncasecmp(command, "PUT", 3)) {

        }
        if (!strncasecmp(command, "QUIT", 4)) {
            status = 0;
        }
        if (!strncasecmp(command, "HELP", 4)) {
            printf("You can enter the following commands: LIST, GET, PUT, and QUIT\n");
        }
        else{
            printf("Invalid command. Type \"HELP\" for more options.\n");
        }
        //printf("%s\n", command);
    }
    printf("Shutting down...\n");
}


int main(int argc, char *argv[], char **envp)
{
    char buf[256];
    char a;
    if(argv[1])
    {
        parseConfFile(argv[1]);
    }
    read_user_input();
}