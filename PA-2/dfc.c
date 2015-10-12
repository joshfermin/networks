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
            confdfc.username[strlen(confdfc.username)-1] = '\0';
        } 
        if (!strcmp(head, "Password:")) {
            sscanf(tail, "%s", confdfc.password);
            confdfc.password[strlen(confdfc.password)-1] = '\0';
        }
    }
    fclose(conf_file);
}

int main(int argc, char *argv[], char **envp)
{
    char buf[256];
    char a;
    if(argv[1])
    {
        parseConfFile(argv[1]);
    }
    while(scanf("%c", &a) == 1)
    {
        printf("Enter command: ");                                                                                                                                                                
        scanf("%s", buf); 
        printf("the comand you entered is: %s", buf)
    } 
}