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
    n = do_write(socket, buffer, strlen(buffer), \
	      "ERROR: Could not write ack message"); // make sure to send null terminator
    printf("Sent %d bytes, message %s\n", n, buffer);
    printf("message length is %d\n", message_length);

    if ((message = malloc(message_length)) == NULL)
	error("ERROR: Could not reallocate memory for message");
    
    n = do_read(socket, buffer, BUFFERLENGTH, \
		"ERROR: Could not read initial part of message");
    remaining -= n;
    printf("Got %d bytes, buffer is %s. %d of %d bytes not yet received.\n", n, buffer, remaining, message_length);
    sprintf(message, buffer, BUFFERLENGTH);
    printf("wrote buffer into message string, %s\n", message);
    while (remaining != 0){
	// Move the pointer in the message string to point to the first unsent character.
	printf("Reading remaining %d bytes.\n", remaining);
	n = do_read(socket, buffer, BUFFERLENGTH, \
		"ERROR: Could not read initial part of message");
	printf("Received %d bytes. Buffer is %s\n", n, buffer);
	strcpy(message + strlen(message), buffer);
	printf("copied received into message: %s\n", message);
	remaining -= n;
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
    int n, remaining = strlen(message) + 1;
    char buffer[BUFFERLENGTH];
    
    sprintf(buffer, "%d", strlen(message) + 1); // null terminator
    // Send the length of the message to be sent.
    n = do_write(socket, buffer, strlen(buffer) + 1, \
		 "ERROR: Could not write message length"); // don't forget null terminator
    
    printf("Sent %d bytes, message %s\n", n, buffer);
        
    // Read the response - should be the same data that was sent.
    n = do_read(socket, buffer, BUFFERLENGTH, \
		"ERROR: Could not read ack message");

    printf("Got %d bytes, message %s\n", n, buffer);

    /*
     * Put the first BUFFERLENGTH -1 characters of the message into the buffer,
     * and cap off with a null terminator.
     */
    snprintf(buffer, BUFFERLENGTH, "%s", message);

    printf("Put %d characters into the buffer (not including null terminator), buffer is %s\n", strlen(buffer), buffer);
    
    // Don't add the null terminator - only want to count the one on the end of the whole message.
    n = do_write(socket, buffer, strlen(buffer) + 1,			\
		 "ERROR: Could not write initial part of message");	
    
    remaining -= n;
    
    printf("%d of %d bytes not yet sent.\n", remaining, strlen(message) + 1);
    if (remaining != 0 ){
	
    }
    
    while (remaining != 0){
	// Move the pointer in the message string to point to the first unsent character.
	printf("message points to %c, whole string: %s\n", *message, message);
	message += n;
	printf("message now points to %c, whole string: %s\n", *message, message);
	// Put the rest of the 
	int chars_to_write = remaining < BUFFERLENGTH ? remaining : BUFFERLENGTH;
	
	printf("writing %d characters\n", chars_to_write);

	snprintf(buffer, chars_to_write, "%s", message);
	printf("n is %d. Remaining bytes: %d, buffer is %s\n", n, remaining, buffer);
	n = do_write(socket, buffer, strlen(buffer) + 1, \
		     "ERROR: Could not write remainder of message.");
		
	remaining -= n;
	sleep(2);
    }
    
        
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
