#include "logclient.h"
#include "loggeneral.h"

#include <signal.h>
#include <time.h>

int terminated = 0;

int main(int argc, char *argv[])
{
    
    int sockfd, portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
    char buffer[STDIN_BUFFERLENGTH];
    if (argc < 3){
	fprintf(stderr, "usage: %s hostname port\n", argv[0]);
	exit(1);
    }

    portno = atoi(argv[2]);
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
	error("ERROR opening socket");
    
    // gets the name of the host
    server = gethostbyname(argv[1]);
    if (server == NULL){
	fprintf(stderr, "ERROR, no such host\n");
	exit(1);
    }
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    /*
     * bcopy (const void *src, void *dest, int n)
     * copy the first n bytes of src to dest.
     * s_addr records the host address number as a uint32
     */
    memcpy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
    // sin_port is the IP port. Change it to the network byte order.
    serv_addr.sin_port = htons(portno);
    

    //srand48(time(NULL));
    /* 
     * connect (int socket, struct sockaddr *addr, socklen_t length)
     * initiates a connection from socket to the socket whose address is specified by
     * the addr and length arguments.
     */

    struct sigaction handler;
    handler.sa_handler = sig_handler;
    handler.sa_flags = 0;
    sigfillset(&handler.sa_mask);
    
    check_handler_setup(sigaction(SIGINT, &handler, NULL), "Failed to set up SIGINT handler");

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	error("Error connecting");
    int valid = 0;
    int ret;
        
    while(1){	
	printf("Please enter the message.\n");
	bzero(buffer, STDIN_BUFFERLENGTH);
	// Messages that exceed the length of the stdin_bufferlength will be truncated.
	if (fgets(buffer, STDIN_BUFFERLENGTH, stdin) == NULL || terminated == 1){
	    printf("Received EOF - exiting.\n");
	    exit(0);
	}

	// get rid of newline on the end of terminal-entered input
	if (*(buffer + strlen(buffer) - 1) == '\n')
	    *(buffer + strlen(buffer) - 1) = '\0';

	if ((ret = send_message(buffer, sockfd)) < 0){
	    printf("Terminating...\n");
	    exit(0);
	}
	
	valid = receive_message_valid(sockfd);
		
	printf("Message %s was %s.\n", buffer, valid == 0 ? "invalid" : "valid");
	printf("\n");
    }
    
    return 0;
}

void sig_handler(int signum)
{
    send_term_message();
    printf("Got term signal.\n");	
    terminated = 1;
}
