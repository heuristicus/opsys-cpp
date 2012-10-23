#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#define BUFFERLENGTH 32
#define STDIN_BUFFERLENGTH 512

// util.c
void error(char *msg);

// sock_util.c
char* receive_message(int socket);
void send_message(char *message, int socket);
int do_write(int socket, char *buffer, int length, char *err_msg);
int do_read(int socket, char *buffer, int length, char *err_msg);
