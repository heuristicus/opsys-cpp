CC=gcc
CFLAGS=-Wall -Werror -g -D _GNU_SOURCE
LIB=-pthread
SOURCES=util.c sock_util.c
CLIENTSOURCES=logclient.c
SERVERSOURCES=logserver.c
CSOURCES=$(SOURCES:.c=.o)
CLIENTOBJECTS=$(CLIENTSOURCES:.c=.o)
SERVEROBJECTS=$(SERVERSOURCES:.c=.o)
CLIENT=logclient
SERVER=logserver

all: client server

client: $(OBJECTS)
	$(CC) $(CFLAGS) $(SOURCES) $(CLIENTSOURCES) -o $(CLIENT) $(LIB)

server: $(OBJECTS)
	$(CC) $(CFLAGS) $(SOURCES) $(SERVERSOURCES) -o $(SERVER) $(LIB)

clean:
	rm -rf $(CLIENTOBJECTS) $(SERVEROBJECTS) $(CSOURCES) $(CLIENT) $(SERVER)
