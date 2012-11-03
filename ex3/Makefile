CFLAGS=-Wall -Werror -g -D _GNU_SOURCE
CC=gcc $(CFLAGS)
LIBS=-lnetfilter_queue -lpthread

all: netqueue ratelimit

ratelimit: ratelimit.o util.o
	$(CC) $< util.o $(LIBS) -o $@

ratelimit.o: ratelimit.c limiter.h
	$(CC) -c $<

netqueue: netqueue.o util.o
	$(CC) $< util.o $(LIBS) -o $@

netqueue.o: netqueue.c limiter.h
	$(CC) -c $<

util.o: util.c limiter.h
	$(CC) -c $<

clean:
	rm -f *.o netqueue ratelimit
