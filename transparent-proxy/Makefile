all: transparent_proxy echo_server

transparent_proxy: transparent_proxy.c
	gcc -W -Wall -o transparent_proxy transparent_proxy.c

echo_server: echo_server.c
	gcc -W -Wall -o echo_server echo_server.c

clean:
	rm transparent_proxy echo_server