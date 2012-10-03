#From http://mrbook.org/tutorials/make/

CC=gcc
CFLAGS=-Wall -Werror -g
SOURCES=lsdir.c strlist.c strfunc.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=lsdir


all: lsdir $(EXECUTABLE)

lsdir:$(OBJECTS)
	$(CC) $(CFLAGS) $(SOURCES) -o $@

clean:
	rm -rf $(OBJECTS)