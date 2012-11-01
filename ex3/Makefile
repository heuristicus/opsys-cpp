CFLAGS=-Wall -Werror
LIBS=-lnetfilter_queue

all: netqueue

netqueue: netqueue.o
	$(CC) $(CFLAGS) -o "$@" $(LIBS) $<

netqueue.o: netqueue.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o netqueue
