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
char* read_message(int *socket)
{
    int n;
    char *buffer = malloc(INITIAL_BUFF_LEN);
    
        
    // read the size of the message to be received into the buffer.
    n = read(*socket, buffer, INITIAL_BUFF_LEN - 1);
    if (n < 0)
	error("Could not read initial message from socket.");
    
    // reallocates the buffer to the size of the message to be received.
    int m_size = atoi(buffer);
    buffer = realloc(buffer, m_size);
    if (buffer == NULL)
	error("Failed to reallocate memory to store message.");
    

    // Send the acknowledgment. Since the string is in buffer, just send that back.
    n = write(*socket, buffer, DEFAULT_STR_INT_SIZE);
    if (n < 0)
	error("Failed to send acknowledgment string.");
    
    // Read some bytes from the socket.
    n = read(*socket, buffer, m_size - 1);
    if (n < 0)
	error("Failed to read anything from the socket.");
	
    int received = n; // add the bytes received to our sum
    
    if (received < m_size) {
	char *size_str = malloc(DEFAULT_STR_INT_SIZE);
	char *tmp = malloc(m_size);
	
	// Loop until we receive the complete message
	while (received < m_size){
	    snprintf(size_str, DEFAULT_STR_INT_SIZE, "%d", m_size - received);
	    // Write the number of bytes that we have yet to receive.
	    n = write(*socket, size_str, DEFAULT_STR_INT_SIZE);
	    if (n < 0)
		error("Failed to write number of unreceived bytes.");
	    
	    // Read the next set of bytes from the socket.
	    n = read(*socket, tmp, m_size - 1);
	    if (n < 0)
		error("Failed to read anything from the socket.");
	    
	    // copy the part of the string that we received into the buffer.
	    strncpy(buffer + received, tmp, n);
	    received += n;
	}
	
    	free(tmp);
	free(size_str);
    }

    // Send a message containing zero - we have received everything.
    n = write(*socket, "0", 2);
    if (n < 0)
	error("Failed to send transfer completed message.");
        
    return buffer; // Remember to free this when you're done!
}
