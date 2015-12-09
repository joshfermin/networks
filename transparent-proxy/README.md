#Proxy
To run:
```bash
make clean
make
./echo_server <port number>
./transparent_proxy <port number>
```

###Design
This proxy consists of 3 main functions
* listenOnPort(int port)
* accept_request(int socket)
* process_get(char* host, int sock, char* request)
There is also an echo server to be used on the server. The client will talk to this echo server via telnet and the proxy.

####listenOnPort(int port)
* This function was copied over from the previous assignments. Basically it opens a socket on localhost with a given port. 
* Whenever this function is called, it forks so that multiple connections can access the server.
* The fork then performs accept_request.

####accept_request(int socket)
* This function reads input in from the user. It should look like the following:
```bash
GET http://www.yahoo.com/ HTTP/1.0
```
* This function parses the method (GET), host (http://www.yahoo.com), and http version (HTTP/1.0) from the user.
* It then checks if the method is a GET request, and if so pass the host in to the process_get function.

####process_get(char* host, int sock, char* request)
* This overall goal of this function is to do a dns lookup on the given host and then when you have the ip of that host name, do a get request on that ip and serve it back to the client.