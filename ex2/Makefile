CFLAGS=-Wall -Werror -g -D _GNU_SOURCE
CC=gcc $(CFLAGS)
LIB=-pthread

all: logclient logserver

logclient: logclient.o sock_util.o loggeneral.h logclient.h 
	$(CC) logclient.o sock_util.o -o $@

logserver: logserver.o sock_util.o threadlist.o loggeneral.h logserver.h
	$(CC) logserver.o sock_util.o threadlist.o $(LIB) -o $@

logserver.o: logserver.c sock_util.o threadlist.o loggeneral.h logserver.h
	$(CC) -c $< -o $@

sock_util.o: sock_util.c loggeneral.h
	$(CC) -c $< -o $@

threadlist.o: threadlist.c logserver.h
	$(CC) -c $< -o $@

clean:
	rm -f logclient logserver *.o
