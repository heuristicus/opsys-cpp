#include "loggeneral.h"

#define INITIAL_BUFF_LEN 20
#define DEFAULT_STR_INT_SIZE 20

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
    int n;
    char *buffer = malloc(INITIAL_BUFF_LEN);
    
        
    // read the size of the message to be received into the buffer.
    n = read(socket, buffer, INITIAL_BUFF_LEN - 1);
    if (n < 0)
	error("Could not read initial message from socket.");
    
    // reallocates the buffer to the size of the message to be received.
    int m_size = atoi(buffer);
    buffer = realloc(buffer, m_size);
    if (buffer == NULL)
	error("Failed to reallocate memory to store message.");
    

    // Send the acknowledgment. Since the string is in buffer, just send that back.
    n = write(socket, buffer, DEFAULT_STR_INT_SIZE);
    if (n < 0)
	error("Failed to send acknowledgment string.");
    
    // Read some bytes from the socket.
    n = read(socket, buffer, m_size - 1);
    if (n < 0)
	error("Failed to read anything from the socket.");
    printf("n is %d\n", n);
    int received = n; // add the bytes received to our sum
    
    if (received < m_size) {
	char *size_str = malloc(DEFAULT_STR_INT_SIZE);
	char *tmp = malloc(m_size);
	
	// Loop until we receive the complete message
	while (received < m_size){
	    assert(received <= m_size); // make sure that something weird isn't happening.
	    printf("Received %d bytes of %d total.\n", received, m_size);
	    snprintf(size_str, DEFAULT_STR_INT_SIZE, "%d", m_size - received);
	    // Write the number of bytes that we have yet to receive.
	    n = write(socket, size_str, DEFAULT_STR_INT_SIZE);
	    if (n < 0)
		error("Failed to write number of unreceived bytes.");
	    
	    // Read the next set of bytes from the socket.
	    n = read(socket, tmp, m_size - 1);
	    if (n < 0)
		error("Failed to read anything from the socket.");
	    
	    // copy the part of the string that we received into the buffer.
	    strncpy(buffer + received, tmp, n);
	    received += n;

	}
	
    	free(tmp);
	free(size_str);
    }
    printf("buffer %s\n", buffer);

    // Send a message containing zero - we have received everything.
    n = write(socket, "0", 2);
    if (n < 0)
	error("Failed to send transfer completed message.");
        
    return buffer; // Remember to free this when you're done!
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
    int size = strlen(message) + 1;// +1 for null pointer
    char *size_str = malloc(DEFAULT_STR_INT_SIZE);
    char *buffer = malloc(DEFAULT_STR_INT_SIZE);
    int n;
    
    snprintf(size_str, DEFAULT_STR_INT_SIZE, "%d", size);
    
    // Send a message containing the size of the message to send.
    n = write(socket, size_str, DEFAULT_STR_INT_SIZE);
    if (n < 0)
	error("Failed to send initial message.");
    
    // Read the size sent back.
    n = read(socket, buffer, DEFAULT_STR_INT_SIZE);
    if (n < 0)
	error("Failed to get a response to initial message.");
    
    assert(atoi(buffer) == size); // make sure the size received is the same as the sent size
    
    n = write(socket, message, size);
    if (n < 0)
	error("Failed to send initial chunk of message.");
    
    n = read(socket, buffer, DEFAULT_STR_INT_SIZE);
    if (n < 0)
	error("Failed to receive a response after sending part of the message.");
    
    // While the "transfer complete" message is not received, keep sending.
    while(atoi(buffer) != 0){
	// We should not come in here. Message should be sent all in one go.
	n = write(socket, message, size);
	if (n < 0)
	    error("Failed to send remainder of message.");
    
	n = read(socket, buffer, DEFAULT_STR_INT_SIZE);
	if (n < 0)
	    error("Failed to receive a response after sending remainder of the message.");
    }

    return 0;
}

