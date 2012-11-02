CFLAGS=-Wall -Werror -g -D _GNU_SOURCE
CC=gcc $(CFLAGS)
LIBS=-lnetfilter_queue

all: netqueue ratelimit

ratelimit: ratelimit.o
	$(CC) $< $(LIBS) -o $@

ratelimit.o: ratelimit.c
	$(CC) -c $<

netqueue: netqueue.o
	$(CC) $< $(LIBS) -o $@

netqueue.o: netqueue.c
	$(CC) -c $<

clean:
	rm -f *.o netqueue
