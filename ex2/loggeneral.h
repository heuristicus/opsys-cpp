#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#define BUFFERLENGTH 256

// util.c
void error(char *msg);

// comms.c
char* receive_message(int socket);
int send_message(char *message, int socket);
