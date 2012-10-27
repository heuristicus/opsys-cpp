#include "loggeneral.h"
#include <pthread.h>
#include <signal.h>

#define TERM_STRING "!\"£$term$£\"!"
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
    char *buffer = calloc(BUFFERLENGTH, sizeof(char));
    char *message;
    printf("Receiving message...\n");
    
    // Read the length of the message to be received, in bytes.
    n = do_read(socket, buffer, BUFFERLENGTH, \
		"ERROR: Did not receive message length");

    if (n == 0 || atoi(buffer) == EOF){
	free(buffer);
	return "EOF";
    }

    //printf("Got %d bytes, message %s\n", n, buffer);
        
    int message_length = atoi(buffer);
    remaining = message_length;
    //printf("Message length %d.\n", message_length);

    // Send back the same message as acknowledgment
    n = do_write(socket, buffer, strlen(buffer) + 1, \
	      "ERROR: Could not write ack message"); // make sure to send null terminator
    //printf("Sent %d bytes, message %s\n", n, buffer);
    //printf("message length is %d\n", message_length);

    if ((message = malloc(message_length)) == NULL)
	error("ERROR: Could not reallocate memory for message");
    
    n = do_read(socket, buffer, BUFFERLENGTH, \
		"ERROR: Could not read initial part of message");

    if (n == 0 || atoi(buffer) == EOF){
	free(buffer);
	free(message);
	return "EOF";
    }

    if (remaining > BUFFERLENGTH)
	remaining -= n - 1;
    else
	remaining -= n;
    
    //printf("Got %d bytes, buffer is %s. %d of %d bytes not yet received.\n", n, buffer, remaining, message_length);

    sprintf(message, buffer, BUFFERLENGTH);
    //printf("wrote buffer into message string, %s\n", message);
    while (remaining != 0){
	//printf("%d of %d bytes not yet received.\n", remaining, message_length);
	// Move the pointer in the message string to point to the first unsent character.
	//printf("Reading remaining %d bytes.\n", remaining);
	n = do_read(socket, buffer, BUFFERLENGTH, \
		"ERROR: Could not read initial part of message");
	
	if (n == 0 || atoi(buffer) == EOF){
	    free(message);
	    free(buffer);
	    return "EOF";
	}

	//printf("Received %d bytes. Buffer is %s\n", n, buffer);
	strcpy(message + strlen(message), buffer);
	//printf("copied received into message: %s\n", message);
	if (remaining > BUFFERLENGTH)
	    remaining -= n - 1;
	else
	    remaining -= n;
    }

    sprintf(buffer, "%d", remaining);
    
    n = do_write(socket, buffer, strlen(buffer) + 1, \
		 "ERROR: Failed to send transfer complete message.");
    
    printf("Message transfer complete. %d bytes received.\n", message_length);

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
    int message_length = strlen(message) + 1;
    int n, remaining = message_length;
    char buffer[BUFFERLENGTH];
    printf("Sending message %s...\n", message);
    
    sprintf(buffer, "%d", message_length); // null terminator
    // Send the length of the message to be sent.
    n = do_write(socket, buffer, strlen(buffer) + 1, \
		 "ERROR: Could not write message length"); // don't forget null terminator
    
    //printf("Sent %d bytes, message %s\n", n, buffer);
        
    // Read the response - should be the same data that was sent.
    n = do_read(socket, buffer, BUFFERLENGTH, \
		"ERROR: Could not read ack message");

    if (n < 1){
	return -1;
    }

    //printf("Got %d bytes, message %s\n", n, buffer);

    /*
     * Put the first BUFFERLENGTH -1 characters of the message into the buffer,
     * and cap off with a null terminator.
     */
    snprintf(buffer, BUFFERLENGTH, "%s", message);

    //printf("Put %d characters into the buffer (not including null terminator), buffer is %s\n", strlen(buffer), buffer);
    
    // Don't add the null terminator - only want to count the one on the end of the whole message.
    n = do_write(socket, buffer, strlen(buffer) + 1,			\
		 "ERROR: Could not write initial part of message");	
    
    if (remaining > BUFFERLENGTH)
	remaining -= n - 1;
    else
	remaining -= n;
        
    //printf("%d of %d bytes not yet sent.\n", remaining, message_length);
    
    while (remaining != 0){
	// Move the pointer in the message string to point to the first unsent character.
	//printf("message points to %c, whole string: %s\n", *message, message);
	message += n - 1;
	//printf("message now points to %c, whole string: %s\n", *message, message);
	// Put the rest of the 
	int chars_to_write = remaining < BUFFERLENGTH ? remaining : BUFFERLENGTH;
	
	//printf("writing %d characters\n", chars_to_write);

	snprintf(buffer, chars_to_write, "%s", message);
	//printf("n is %d. Remaining bytes: %d, buffer is %s\n", n, remaining, buffer);
	//printf("%d bytes left to send.\n", remaining);
	n = do_write(socket, buffer, strlen(buffer) + 1, \
		     "ERROR: Could not write remainder of message.");
		
	if (remaining > BUFFERLENGTH)
	    remaining -= n - 1;
	else
	    remaining -= n;
	
	//sleep(2);
    }
    
    n = do_read(socket, buffer, BUFFERLENGTH, \
		"ERROR: Could not read transfer complete ack.");

    if (n < 1){
	return -1;
    }

    int serv_rec = atoi(buffer);

    assert(serv_rec == 0);
    
    printf("Message transfer complete. Sent %d bytes.\n", message_length);
            
    return serv_rec;
}

/*
 * Sends a message containing a single integer which contains information
 * about the validity of the message that was received. 0 implies invalidity, 1 validity.
 */ 
void send_message_valid(int message, int socket)
{
    char* v = malloc(sizeof(int));
    
    sprintf(v, "%d", message);
    
    do_write(socket, v, strlen(v),\
	     "ERROR: Could not send validation message.");
    
    printf("Validation message sent successfully.\n");

    free(v);
        
}

/*
 * Receives a message containing an integer which contains information about
 * the validity of the message that was sent. 0 implies invalidity, 1 validity
 */ 
int receive_message_valid(int socket)
{
    printf("Receiving validation message...\n");
    
    char v[2] = "";
        
    do_read(socket, v, 2, \
	    "ERROR: Could not read validation message.");
    
    printf("v is %s\n", v);

    return atoi(v);
}

/*
 * Helper method for reading from a socket.
 * Will exit the program with the given error message if something fails.
 * If a termination message is received the thread will also exit.
 */
int do_read(int socket, char *buffer, int length, char *err_msg)
{
    int n;
    n = read(socket, buffer, length);
    if (n < 0)
	error(err_msg);

    if (n == 0){
	printf("Received EOF. Exiting.\n");
	return n;
    }

    if (memcmp(buffer, TERM_STRING, min(strlen(TERM_STRING), strlen(buffer))) == 0 && strlen(buffer) != 0){
	printf("TERM MESSAGE RECEIVED\n");
	return 0;
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

    if (n == 0)
	printf("got EOF\n");


    return n;
}

// Sends a termination message to the specified socket.
void send_term_message(int socket)
{
    write(socket, TERM_STRING, strlen(TERM_STRING));
}


// Returns the smaller of two integers
int min(int x, int y)
{
    return x < y ? x : y;
}

// Checks whether a signal handler has been set up correctly.
void check_handler_setup(int result, char *msg)
{
    if (result != 0)
	error(msg);
}

// Prints an error message.
void error(char *msg)
{
    perror(msg);
}
