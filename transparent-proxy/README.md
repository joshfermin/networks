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

There is also an echo server to be used on the server vm. The client will talk to this echo server via telnet and the proxy server.

####listenOnPort(int port)
* This function was copied over from the previous assignments. Basically it opens a socket on localhost with a given port. 
* Whenever this function is called, it forks so that multiple connections can access the server.
* It also performs a dnat.
* The fork then performs accept_request.

####accept_request(int socket)
* This function reads input in from the user. It does snat and then logs to the file logging.txt


###How To Use
* Run the proxy on the proxy server (VM2)
* Run the echo server on the server (VM3)
* Be on the client (VM1) and telnet to the server (VM3)
```bash
telnet 10.0.0.2 
```