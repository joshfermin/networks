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
#include "conf.h"
#include "httprequest.h"

#define ISspace(x) isspace((int)(x))

void accept_request(int);
int get_line(int, char *, int);
void serve_file(int, const char *);
void cat(int, FILE *);
void headers(int client, const char *filename);
char * deblank(char *string);
void error404(int, const char *filename);
void error400(int, char* type);
void error500(int);
void error501(int, const char *filename);
int isInvalidURI(char * uri);


void accept_request(int client)
{
	char *ext;
	char buf[1024];
	int numchars;
	char method[255];
	char url[255];
	char path[512];
	size_t i, j;
	struct stat st;
	char *query_string = NULL;

	numchars = get_line(client, buf, sizeof(buf));
	i = 0; j = 0;

	while ((!ISspace(buf[j]) && i < sizeof(http_request.method) - 1))
	{
		http_request.method[i] = buf[j];
		i++; j++;
	}
	http_request.method[i] = '\0';

	// sscanf(method, "%s %s %s", http_request.method, http_request.path, http_request.http_version);
	// http_request.method[strlen(http_request.method)-1] = '\0';

	i = 0;
	while (ISspace(buf[j]) && (j < sizeof(buf)))
		j++;
		while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))
		{
			url[i] = buf[j];
			i++; j++;
		}
	url[i] = '\0';


	i=0;
	while (ISspace(buf[j]) && (j < sizeof(buf)))
		j++;
		while (!ISspace(buf[j]) && (i < sizeof(http_request.http_version) - 1) && (j < sizeof(buf)))
		{
			http_request.http_version[i] = buf[j];
			i++; j++;
		}
	http_request.http_version[i] = '\0';
	printf("%s\n", http_request.http_version);
	// printf("url is: %s\n", url);
	
	if (strcasecmp(http_request.method, "GET") == 0)
	{
		query_string = url;
		while ((*query_string != '?') && (*query_string != '\0'))
		 query_string++;
		if (*query_string == '?')
		{
		 *query_string = '\0';
		 query_string++;
		}
	}
	// printf("%s", query_string);




	// printf("%s", url);
	// parsing url to see file extension
	ext = strrchr(url, '.');
	if (ext != NULL) {
		// printf("comparing contentType: %s with ext: %s", contentType, ext);
		if(strstr(conf.contentType, ext) == NULL)
		{
			error501(client, url);
			close(client);
			// return;
		}
	}
	// 400 error handling
	else if (strcasecmp(http_request.method, "POST") == 0)
	{
		error400(client, "Invalid Method");
		// close(client);
	}
	if (strcmp(http_request.http_version, "HTTP/1.1") != 0)
	{
		error400(client, "Invalid Version");
	}
	else if (isInvalidURI(url))
	{
		error400(client, "Invalid URI");
	}

	// printf("contentType is: %s \n", conf.contentType);
	// printf("document_root is: %s \n", conf.document_root);
	// printf("directoryIndex is: %s \n", conf.DirectoryIndex);

	sprintf(path, "%s%s", conf.document_root, url);
	// printf("%s ", path);
	if (path[strlen(path)] == '/')
	strcat(path, "index.html");
// printf("%s", path);
	if (stat(path, &st) == -1) {
		while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
			numchars = get_line(client, buf, sizeof(buf));
			error404(client, path);
	}
	else
	{
	if ((st.st_mode & S_IFMT) == S_IFDIR)
	 strcat(path, "/index.html");
	 // strcat(path,directoryIndex);
	 serve_file(client, path);
	}

	close(client);
}

// sends a file to the client
void serve_file(int client, const char *filename)
{
	// printf("%s", filename);
	char *sendbuf;
	FILE *requested_file;
	long fileLength;
	printf("Received request for file: %s on socket %d\n", filename + 1, client);
	
	// if (fileSwitch) { requested_file = fopen(file + 1, "rb"); }
	
	requested_file = fopen(filename, "rb");
	
	if (requested_file == NULL)
	{
		error404(client, filename);
	}
	if(strstr(filename, ".html") != NULL)
	{
		FILE *resource = NULL;
		int numchars = 1;
		char buf[1024];

		buf[0] = 'A'; buf[1] = '\0';
		while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
			numchars = get_line(client, buf, sizeof(buf));

		resource = fopen(filename, "r");
		headers(client, filename);
		cat(client, resource);
		fclose(resource);
	}
	 else
	{
		// printf("im in else");
		fseek (requested_file, 0, SEEK_END);
		fileLength = ftell(requested_file);
		rewind(requested_file);
		
		sendbuf = (char*) malloc (sizeof(char)*fileLength);
		size_t result = fread(sendbuf, 1, fileLength, requested_file);
		
		if (result > 0) 
		{
			headers(client, filename);
			send(client, sendbuf, result, 0);   
		}   
		else { error500(client); }
	}
	
	fclose(requested_file);
	
}

int isInvalidURI(char * uri)
{
	// if(strstr())
	return 0;
}

void cat(int client, FILE *resource)
{
 char buf[1024];

 fgets(buf, sizeof(buf), resource);
 while (!feof(resource))
 {
	send(client, buf, strlen(buf), 0);
	fgets(buf, sizeof(buf), resource);
 }
}
// provides the client with the correct headers
// when a request is sent
void headers(int client, const char *filename)
{
	char buf[1024];
	(void)filename;  /* could use filename to determine file type */
	const char* filetype;
	struct stat st; 
	off_t size;

	if (stat(filename, &st) == 0)
		size = st.st_size;

	const char *dot = strrchr(filename, '.');
	if(!dot || dot == filename) {
		filetype = "";
	} else {
		filetype = dot + 1;
	}
	if(strcmp(filetype, "html") == 0)
	{
		strcpy(buf, "HTTP/1.1 200 OK\r\n");
		send(client, buf, strlen(buf), 0);
		sprintf(buf, "Content-Length: %zd \r\n", size);
		send(client, buf, strlen(buf), 0);
		sprintf(buf, "Connection: Keep-Alive \r\n");
		send(client, buf, strlen(buf), 0);
		sprintf(buf, "Content-Type: text/html\r\n");
		send(client, buf, strlen(buf), 0);
		strcpy(buf, "\r\n");
		send(client, buf, strlen(buf), 0);
	}

	else if(strcmp(filetype, "txt") == 0)
	{
		strcpy(buf, "HTTP/1.1 200 OK\r\n");
		send(client, buf, strlen(buf), 0);
		sprintf(buf, "Content-Type: text/plain\r\n");
		send(client, buf, strlen(buf), 0);
		sprintf(buf, "Content-Length: %zd \r\n", size);
		send(client, buf, strlen(buf), 0);
		strcpy(buf, "\r\n");
		send(client, buf, strlen(buf), 0);
	}
	else if (strcmp(filetype, "png") == 0)
	{
		strcpy(buf, "HTTP/1.1 200 OK\r\n");
		send(client, buf, strlen(buf), 0);
		sprintf(buf, "Content-Type: image/png\r\n");
		send(client, buf, strlen(buf), 0);
		sprintf(buf, "Content-Length: %zd \r\n", size);
		send(client, buf, strlen(buf), 0);
		sprintf(buf, "Content-Transfer-Encoding: binary\r\n");
		send(client, buf, strlen(buf), 0);
		strcpy(buf, "\r\n");
		send(client, buf, strlen(buf), 0);
	}

	else if (strcmp(filetype, "gif") == 0)
	{
		strcpy(buf, "HTTP/1.1 200 OK\r\n");
		send(client, buf, strlen(buf), 0);
		sprintf(buf, "Content-Type: image/gif\r\n");
		send(client, buf, strlen(buf), 0);
		sprintf(buf, "Content-Length: %zd \r\n", size);
		send(client, buf, strlen(buf), 0);
		sprintf(buf, "Content-Transfer-Encoding: binary\r\n");
		send(client, buf, strlen(buf), 0);
		strcpy(buf, "\r\n");
		send(client, buf, strlen(buf), 0);
	}

}

char * deblank(char *str)
{
	char *out = str, *put = str;

	for(; *str != '\0'; ++str)
	{
		if(*str != ' ')
			*put++ = *str;
	}
	*put = '\0';

	return out;
}

int get_line(int sock, char *buf, int size)
{
	int i = 0;
	char c = '\0';
	int n;

	while ((i < size - 1) && (c != '\n'))
	{
		n = recv(sock, &c, 1, 0);
		/* DEBUG printf("%02X\n", c); */
		if (n > 0)
		{
		 if (c == '\r')
		 {
			n = recv(sock, &c, 1, MSG_PEEK);
			/* DEBUG printf("%02X\n", c); */
			if ((n > 0) && (c == '\n'))
			 recv(sock, &c, 1, 0);
			else
			 c = '\n';
		 }
		 buf[i] = c;
		 i++;
		}
		else
		 c = '\n';
	}
	buf[i] = '\0';

	return(i);
}

// tell client that content requested was not found on server
void error404(int client, const char *filename)
{
	char buf[1024];

	sprintf(buf, "HTTP/1.1 404 NOT FOUND\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<HTML><TITLE>404 NOT FOUND</TITLE>\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<BODY><P>HTTP/1.1 404 Not Found: %s \r\n", filename);
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "</BODY></HTML>\r\n");
	send(client, buf, strlen(buf), 0);
}

// tell client that they have made a bad request.
void error400(int client, char *type)
{
	char buf[1024];

	sprintf(buf, "HTTP/1.1 400 BAD REQUEST\r\n");
	send(client, buf, sizeof(buf), 0);
	sprintf(buf, "Content-type: text/html\r\n");
	send(client, buf, sizeof(buf), 0);
	sprintf(buf, "\r\n");
	send(client, buf, sizeof(buf), 0);
	if(strcmp(type, "Invalid Method"))
	{
	 sprintf(buf, "<P>HTTP/1.1 400 Bad Request:  Invalid Method");
	 send(client, buf, sizeof(buf), 0);
	}
	sprintf(buf, "\r\n");
	send(client, buf, sizeof(buf), 0);
}

void error500(int client)
{
	char buf[1024];

	sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "HTTP/1.1 500  Internal  Server  Error:  cannot  allocate  memory\r\n");
	send(client, buf, strlen(buf), 0);
}

void error501(int client, const char *filename)
{
	char buf[1024];

	sprintf(buf, "HTTP/1.1 501 Method Not Implemented\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "</TITLE></HEAD>\r\n");
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "<BODY><P>HTTP/1.1 501  Not Implemented:  %s\r\n", filename);
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "</BODY></HTML>\r\n");
	send(client, buf, strlen(buf), 0);
}

// zhued parser copyright 2015. Thanks man!
void parseConfFile(const char *filename)
{
	char *line;
    char head[64], tail[256];
    size_t len = 0;
    int read_len = 0;

    FILE* conf_file = fopen(filename, "r");
    while((read_len = getline(&line, &len, conf_file)) != -1) {

        line[read_len-1] = '\0';
        if (line[0] == '#')
            continue;
        sscanf(line, "%s %s", head, tail);
        if (!strcmp(head, "Listen")) {
            conf.port = atoi(tail);
        } 
        if (!strcmp(head, "DocumentRoot")) {
            sscanf(line, "%*s%s", conf.document_root);
            conf.document_root[strlen(conf.document_root)-1] = '\0';
        } 
        if (!strcmp(head, "DirectoryIndex")) {
            sscanf(line, "%*s %s", conf.DirectoryIndex);
            conf.document_root[strlen(conf.DirectoryIndex)-1] = '\0';
        }
        if (head[0] == '.') {
            strcat(conf.contentType, head);
            strcat(conf.contentType, " ");
        }
    }
    fclose(conf_file);
}


int main()
{
	parseConfFile("ws.conf");
	int one = 1, client_fd, status;
	// pthread_t newthread;

	// sockaddr: structure to contain an internet address.
	struct sockaddr_in svr_addr, cli_addr;
	socklen_t sin_len = sizeof(cli_addr);

	// create endpoint for comm, AF_INET (IPv4 Internet Protocol), 
	// SOCK_STREAM (Provides sequenced, reliable, two-way, connection-based byte streams),
	int sock = socket(AF_INET, SOCK_STREAM, 0); // returns file descriptor for new socket
	// if (sock < 0)
	//   err(1, "cant open socket");

	// (int socket SOL_SOCKET to set options at socket level, allows reuse of local addresses,
	// option value, size of socket)
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));

	svr_addr.sin_family = AF_INET;
	svr_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	svr_addr.sin_port = htons(conf.port); // host to network short (htons)

	// Start server
		if (bind(sock, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) == -1) {
			close(sock);
			// err(1, "Can't bind");
		}
	 
		listen(sock, 5); // 5 is backlog - limits amount of connections in socket listen queue
		printf("listening on localhost with port %d\n", conf.port);


	while (1)
	{
		client_fd = accept(sock,
											 (struct sockaddr *)&cli_addr,
											 &sin_len);
		// printf("got connection\n");
			if(fork() == 0){
				/* Perform the client’s request in the child process. */
				accept_request(client_fd);
				exit(0);
			}
			close(client_fd);

			/* Collect dead children, but don’t wait for them. */
			waitpid(-1, &status, WNOHANG);
		// if (pthread_create(&newthread , NULL, accept_request, client_fd) != 0)
			// perror("pthread_create");
	}
	close(sock);
}