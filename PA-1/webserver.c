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

void accept_request(int client)
{
  FILE* file = fopen("ws.conf", "r");
  char line[256];
  char directoryIndex[100]; 
  char document_root[100];
  char contentType[100];
  char *ext;
  char buf[1024];
  int numchars;
  char method[255];
  char url[255];
  char path[512];
  size_t i, j;
  struct stat st;
  char *query_string = NULL;
  ///////////////////
  ///PARSING LOGIC///
  ///////////////////
  while (fgets(line, sizeof(line), file)) {
        /* note that fgets don't strip the terminating \n, checking its
           presence would allow to handle lines longer that sizeof(line) */
    if(line[0] != '#')
    {
      char* parse = strtok(line, " " );
      while (parse){
      // printf("%s\n", parse);

        if(strcmp(parse, "DirectoryIndex") == 0)
        {
          parse = strtok(NULL, "");
          // strcpy(test, parse);
          strcat(directoryIndex, strtok(deblank(parse),"\n"));

        }
        else if(strcmp(parse, "DocumentRoot") == 0)
        {
          parse = strtok(NULL, " ");
          strcat(document_root, strtok(deblank(parse), "\n"));
        }
        else if(parse[0] == '.')
        {
          strcat(contentType, parse);
          parse = strtok(NULL, ",");
        }
        else
        {
          parse = strtok(NULL, " "); 
        }
        
      }
    }

  } 
          // printf("directoryIndex is: %s\n", directoryIndex);


  fclose(file);
  ///////////////////
  ///////////////////
  ///////////////////

  numchars = get_line(client, buf, sizeof(buf));
  i = 0; j = 0;

  // 
  while (!ISspace(buf[j]) && (i < sizeof(method) - 1))
  {
    method[i] = buf[j];
    i++; j++;
  }
  method[i] = '\0';
  // printf("%s", method);

  i = 0;
  while (ISspace(buf[j]) && (j < sizeof(buf)))
    j++;
    while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))
    {
      url[i] = buf[j];
      i++; j++;
    }
  url[i] = '\0';
  
  if (strcasecmp(method, "GET") == 0)
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


  // 400 error handling
  if (strcasecmp(method, "POST") == 0)
  {
    error400(client, "Invalid Method");
  }
  // if (strstr(url, ""))

  // printf("%s", url);
  // parsing url to see file extension
  ext = strrchr(url, '.');
  if (ext != NULL) {
    // printf("comparing contentType: %s with ext: %s", contentType, ext);
    if(strstr(contentType, ext) == NULL)
    {
      error501(client, url);
      close(client);

      // return;
    }
  }
  sprintf(path, "www%s", url);
  if (path[strlen(path)] == '/')
  strcat(path, directoryIndex);
// printf("%s", path);
  if (stat(path, &st) == -1) {
  while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
   numchars = get_line(client, buf, sizeof(buf));
  error404(client, path);
  }
  else
  {
  if ((st.st_mode & S_IFMT) == S_IFDIR)
   strcat(path,"/index.html");
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
    sprintf(buf, "Content-Length: %lld \r\n", size);
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
    sprintf(buf, "Content-Length: %lld \r\n", size);
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
    sprintf(buf, "Content-Length: %lld \r\n", size);
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



int main()
{
  ////////////////////////////
  // PARSING LOGIC FOR PORT //
  ////////////////////////////
  FILE* file = fopen("ws.conf", "r");
  char line[256];
  int port;

  while (fgets(line, sizeof(line), file)) {
        /* note that fgets don't strip the terminating \n, checking its
           presence would allow to handle lines longer that sizeof(line) */
    if(line[0] != '#')
    {
      char* parse = strtok(line, " " );
      // printf("%s\n", parse);
      while (parse){
        if(strcmp(parse, "Listen") == 0){
          parse = strtok(NULL, " ");
          port = atoi(parse);
        }
        else
        {
          parse = strtok(NULL, " "); 
        }
      }
    }
  }
  fclose(file);
  ///////////////////
  // PARSING LOGIC //
  ///////////////////


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
  svr_addr.sin_port = htons(port); // host to network short (htons)
 
  // Start server
    if (bind(sock, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) == -1) {
      close(sock);
      // err(1, "Can't bind");
    }
   
    listen(sock, 5); // 5 is backlog - limits amount of connections in socket listen queue
    printf("listening on localhost with port %d\n", port);


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