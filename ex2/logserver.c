#include "logserver.h"
#include "loggeneral.h"

char *fname;
int returnValue;
pthread_mutex_t lock;


int main(int argc, char *argv[])
{
    if (argc < 3){
	fprintf(stderr, "usage: %s portnumber filename\n", argv[0]);
	exit(1);
    }
    
    size_t clilen;
    int sockfd, portno;
    struct sockaddr_in serv_addr, cli_addr;
    pthread_t *server_thread;
    int result;

    fname = argv[2];
        
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
    // Set port number that the server listens on
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
	int *newsockfd = malloc(sizeof(int));
	if (!newsockfd)
	    fprintf(stderr, "Failed to allocate memory for a new socket.\n");
	
	*newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	
	if (*newsockfd < 0)
	    error("ERROR on accept");

	server_thread = malloc(sizeof(pthread_t));
	pthread_attr_t pthread_attr;
	
	if (!server_thread){
	    fprintf(stderr, "Could not allocate memory for thread.\n");
	    exit(1);
	}
	
	// initialises the thread object with default attributes.
	if (pthread_attr_init(&pthread_attr)){
	    fprintf(stderr, "Creation of thread attributes failed.\n");
	    exit(1);
	}
	
	/*
	 * detached state frees all thread resources when the thread terminates.
	 * in joinable state, thread resources are kept allocated after it terminates,
	 * but it is possible to synchronise on the thread termination and recover
	 * its termination code.
	 * In this case we set the state to detached..
	 */
	if (pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED)){
	    fprintf(stderr, "Setting thread attributes failed.\n");
	    exit(1);
	}

	/*
	 * int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg);
	 * creates a new thread with the attributes specified in attr. Modifying the values of attr later
	 * will not affect thread behaviour. On successful creation, the ID of the thread is returned.
	 * The thread starts executing start_routine with arg as its only argument.
	 */
	result = pthread_create(server_thread, &pthread_attr, logstring, (void *) newsockfd);
	if (result != 0){
	    fprintf(stderr, "Thread creation failed.\n");
	    exit(1);
	}			
    }

    return 0;
}

/* logs a string of formal <alphanum> : <textchar> to the specified file. */
void* logstring(void *args)
{
    int *newsockfd = (int *) args;
    char buffer[BUFFERLENGTH];
    int n;
        
    /*
     * read (int filedes, void *buffer, size_t size)
     * reads up to size bytes from the file with descriptor filedes into buffer.
     * returns the number of bytes actually read. zero return indicates EOF.
     */
    n = read(*newsockfd, buffer, BUFFERLENGTH - 1);
    if (n < 0)
	error("ERROR reading from socket");
	
    printf("Received a message %s\n", buffer);
    /*
     * write (int filedes, const void *buffer, size_t size)
     * writes size bytes from buffer into the file with descriptor filedes.
     * sockets are treated as files, so this is normal.
     * return is the number of bytes actually written. 
     */
    n = write(*newsockfd, "I got your message", 18);
    if (n < 0)
	error("ERROR writing to socket");
	
    // close the socket and free the memory.
    close(*newsockfd);
    free(newsockfd);
    
    // return some value as the exit status of the thread.
    returnValue = 0;
    pthread_exit(&returnValue);
}