#include "loggeneral.h"
#include <pthread.h>
/*
 * reads a message of any length from socket. Returns the whole message in a single string.
 * The protocol followed is as follows: 
 * The first message received contains the size n in bytes of the message to be sent.
 * The response is a string acknowledging receipt, which is the size that was received.
 * The next message is a number of bytes b, 0 < b <= n.
 * If b < n, a response is sent with the number of bytes left to receive,
 * and this process will continue until the number of bytes read corresponds with
 * the number of bytes that were supposed to be sent.
 * If the number of bytes specified have been received, then a "finished" response
 * is sent and communication is over for the time being.
 */
char* receive_message(int socket)
{
    int n, remaining;
    char *buffer = malloc(BUFFERLENGTH);
    char *message;
        
    // Read the length of the message to be received, in bytes.
    n = do_read(socket, buffer, BUFFERLENGTH, \
		"ERROR: Did not receive message length");

    if (n == 0 || atoi(buffer) == EOF){
	return "EOF";
    }

        
    printf("Got %d bytes, message %s\n", n, buffer);
        
    int message_length = atoi(buffer);
    remaining = message_length;
    
    // Send back the same message as acknowledgment
    n = do_write(socket, buffer, strlen(buffer) + 1, \
	      "ERROR: Could not write ack message"); // make sure to send null terminator
    printf("Sent %d bytes, message %s\n", n, buffer);
    printf("message length is %d\n", message_length);

    if ((message = malloc(message_length)) == NULL)
	error("ERROR: Could not reallocate memory for message");
    
    n = do_read(socket, buffer, BUFFERLENGTH, \
		"ERROR: Could not read initial part of message");
    remaining -= n;
    printf("Got %d bytes, message %s. %d of %d bytes not yet received.\n", n, buffer, remaining, message_length);
    sprintf(message, buffer, BUFFERLENGTH);
    while (remaining != 0){
	printf("Reading remaining %d bytes.\n", remaining);
	n = do_read(socket, buffer, BUFFERLENGTH, \
		"ERROR: Could not read initial part of message");
    }

    free(buffer);
        
    return message;
}


/*
 * sends a message to the specified socket. The protocol is as follows.
 * The first message sent is a message which contains the total size
 * of the data to be sent.
 * The response will echo that value.
 * The first chunk of data is sent to the socket.
 * If the response is 0, all data was received, so stop.
 * Otherwise, send remaining data until the 0 message is received.
 */
int send_message(char *message, int socket)
{
    int n;
    char buffer[BUFFERLENGTH];
    
    // Get rid of newline that comes with entering the message in the terminal.
    /* for (n = 0; n < strlen(message); ++i){ */
    /* 	if (*(message + i) == '\n') */
    /* 	    *(message + i) == '\0'; */
    /* } */
    
    sprintf(buffer, "%d", strlen(message) + 1); // null terminator
    // Send the length of the message to be sent.
    n = do_write(socket, buffer, strlen(buffer) + 1, \
		 "ERROR: Could not write message length"); // don't forget null terminator
    
    printf("Sent %d bytes, message %s\n", n, buffer);
        
    // Read the response - should be the same data that was sent.
    n = do_read(socket, buffer, BUFFERLENGTH, \
		"ERROR: Could not read ack message");

    printf("Got %d bytes, message %s\n", n, buffer);

    // Put the first x characters of the message into the buffer.
    n = snprintf(buffer, BUFFERLENGTH, "%s", message);
    printf("Put %d characters into the buffer (not including null terminator), buffer is %s\n", n, buffer);
    
    n = do_write(socket, buffer, strlen(buffer) + 1, \
		 "ERROR: Could not write initial part of message");
    
    return n;
}

/*
 * Helper method for reading from a socket.
 * Will exit the program with the given error message if something fails.
 */
int do_read(int socket, char *buffer, int length, char *err_msg)
{
    int n;
    n = read(socket, buffer, length);
    if (n < 0)
	error(err_msg);

    if (n == 0){
	printf("Received EOF. Exiting.\n");
	free(buffer);
	pthread_exit(0);
    }
            	
    return n;
}

/*
 * Helper method for writing to a socket.
 * Will exit the program with the given error message if something fails.
 */
int do_write(int socket, char *buffer, int length, char *err_msg)
{
    int n;
    n = write(socket, buffer, length);
    if (n < 0)
	error(err_msg);

    return n;
}
