CC = gcc
CFLAGS = -Wall -g -lpthread -lcrypto

all: dfc dfs

dfc:
	$(CC) $(CFLAGS) dfc.c -o dfc

dfs:
	$(CC) $(CFLAGS) dfs.c -o dfs

clean:
	rm -f dfc dfs *.o *~ core