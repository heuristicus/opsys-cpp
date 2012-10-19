#include "logserver.h"

int main(int argc, char *argv[])
{
    if (argc < 2){
	fprintf(stderr, "ERROR: You must provide a port number to listen on.\n");
	exit(1);
    }
    
    size_t clilen;
    int sockfd, newsockfd, portno;
    char buffer[BUFFERLENGTH];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    
    /*
     * socket(namespace, style, protocol)
     * namespace must be PF_LOCAL or PF_INET
     * style is the style of communication. Stream give you a pipe, for dgram you 
     * must specify recipient address each time.
     * protocol designates a specific protocol (zero is usually OK)
     */
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    
    if (sockfd < 0)
	error("Error opening socket.");
    
    /*
     * bzero initialises memory segment to null. Similar to memset, but memset 
     * can be used to set segment to a specific value.
     */
    bzero ((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    
    // address family (must be AF_INET)
    serv_addr.sin_family = AF_INET;
    /*
     * sin_addr is the IP address, s_addr is an IPv4 address formatted as a u_long
     * INADDR_ANY binds to the default address on the server machine. Can receive
     * packets bound for any of the interfaces, and work without knowing the IP
     * of the machine that it's running on.
     */
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    // changes a uint_16 from the host byte order to the network byte order.
    serv_addr.sin_port = htons(portno);
    
    /*
     * Assign a memory address to the socket. arg 0 specifies the socket to bind an
     * address to, arg 1 specifies the address to put it into, arg 2 specifies the size.
     */
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	error("Error on binding");
    
    /* enables the socket to accept connections - the second argument is the length of the queue. */
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    
    while(1){
	/*
	 * accepts a connection request on the server socket.
	 * accept (int socket, struct sockaddr *addr, socklen_t *length_ptr)
	 * addr and length_ptr are used to return information about the 
	 * name of the client socket.
	 * returns a file descriptor for a new socket which is connected to the client.
	 */
	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	
	if (newsockfd < 0)
	    error("ERROR on accept");
	bzero(buffer, BUFFERLENGTH);
	/*
	 * read (int filedes, void *buffer, size_t size)
	 * reads up to size bytes from the file with descriptor filedes into buffer.
	 * returns the number of bytes actually read. zero return indicates EOF.
	 */
	n = read(newsockfd, buffer, BUFFERLENGTH - 1);
	
	if (n < 0)
	    error("ERROR reading from socket");
	
	printf("Here is message %s\n", buffer);
	/*
	 * write (int filedes, const void *buffer, size_t size)
	 * writes size bytes from buffer into the file with descriptor filedes.
	 * sockets are treated as files, so this is normal.
	 * return is the number of bytes actually written. 
	 */
	n = write(newsockfd, "I got your message", 18);
	if (n < 0)
	    error("ERROR writing to socket");
	
	close(newsockfd);
			
    }
    return 0;
}

