#include "logclient.h"

int main(int argc, char *argv[])
{
    
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
    char buffer[BUFFERLENGTH];
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
    memcpy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h.length);
    // sin_port is the IP port. Change it to the network byte order.
    serv_addr.sin_port = htons(portno);
    
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	error("Error connecting");
    
    printf("Please enter the message.\n");
    bzero(buffer, BUFFERLENGTH);
    fgets(buffer, BUFFERLENGTH, stdin);
    
    n = write(sockfd, buffer, strlen(buffer));
    if (n < 0)
	error("Error writing to socket");

    bzero(buffer, BUFFERLENGTH);
    
    n = read(sockfd, buffer, BUFFERLENGTH - 1);
    if (n < 0)
	error("Error reading from socket");
    printf("%s\n", buffer);
    
    return 0;
}
