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


void parseConfFile(const char *);

void parseConfFile(const char *filename)
{
	char *line;
    char head[64], tail[256];
    size_t len = 0;
    int read_len = 0;
    char usernames[256], passwords[256];

    FILE* conf_file = fopen(filename, "r");
    while((read_len = getline(&line, &len, conf_file)) != -1) {
        // printf("%s",line);
        line[read_len-1] = '\0';
        if (line[0] == '#')
            continue;
        sscanf(line, "%s %s", head, tail);
        strcat(usernames,head);	
        strcat(passwords,tail);
    }
    printf("%s", usernames);
    printf("\n%s",passwords);

    fclose(conf_file);
}

int main(int argc, char *argv[], char **envp)
{
	// Display each command-line argument.
   


    parseConfFile("dfs.conf");
    return 1;
}