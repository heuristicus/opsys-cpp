CFLAGS=-Wall -Werror -g -D _GNU_SOURCE
CC=gcc $(CFLAGS)
LIB=-pthread
SOURCES=util.c sock_util.c
CLIENTSOURCES=logclient.c
SERVERSOURCES=logserver.c
OBJECTS=$(SOURCES:.c=.o)
CLIENTOBJECTS=$(CLIENTSOURCES:.c=.o)
SERVEROBJECTS=$(SERVERSOURCES:.c=.o)

all: logclient logserver

logclient: $(CLIENTOBJECTS) $(OBJECTS)
	$(CC) $(SOURCES) $(CLIENTSOURCES) -o $@ $(LIB)

logserver: $(SERVEROBJECTS) $(OBJECTS)
	$(CC) $(SOURCES) $(SERVERSOURCES) -o $@ $(LIB)

$(OBJECTS): $(SOURCES) loggeneral.h
	$(CC) -c $< -o $@

$(CLIENTOBJECTS): $(CLIENTSOURCES) $(OBJECTS) logclient.h
	$(CC) -c $< -o $@

$(SERVEROBJECTS): $(SERVERSOURCES) $(OBJECTS) logserver.h
	$(CC) -c $< -o $@

clean:
	rm -f $(CLIENTOBJECTS) $(SERVEROBJECTS) $(OBJECTS) logclient logserver
