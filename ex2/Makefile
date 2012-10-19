CC=gcc
CFLAGS=-Wall -Werror -g -D _GNU_SOURCE
LIB=-pthread
CLIENTSOURCES=logclient.c
SERVERSOURCES=logserver.c
CLIENTOBJECTS=$(CLIENTSOURCES:.c=.o)
SERVEROBJECTS=$(SERVERSOURCES:.c=.o)
CLIENT=logclient
SERVER=logserver

all: client server

client: $(OBJECTS)
	$(CC) $(CFLAGS) $(CLIENTSOURCES) -o $(CLIENT) $(LIB)

server: $(OBJECTS)
	$(CC) $(CFLAGS) $(SERVERSOURCES) -o $(SERVER) $(LIB)

clean:
	rm -rf $(CLIENTOBJECTS) $(SERVEROBJECTS) $(CLIENT) $(SERVER)
