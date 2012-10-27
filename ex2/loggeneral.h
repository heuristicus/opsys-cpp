#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#define BUFFERLENGTH 31
#define STDIN_BUFFERLENGTH 512

// sock_util.c
char* receive_message(int socket);
int send_message(char *message, int socket);
int do_write(int socket, char *buffer, int length, char *err_msg);
int do_read(int socket, char *buffer, int length, char *err_msg);
void send_message_valid(int message, int socket);
int receive_message_valid(int socket);
int min(int, int);
void check_handler_setup(int result, char *msg);
void send_term_message();
void error(char *msg);



