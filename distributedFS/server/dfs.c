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

void parseConfFile(const char *);
void processRequest(int);
int countLines(const char *);

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

void processRequest(int connection)
{

}

int main(int argc, char *argv[], char **envp)
{
    parseConfFile("server/dfs.conf");
    return 1;
}