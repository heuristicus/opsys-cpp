#include "logserver.h"
#include <signal.h>

FILE *fp;
int returnValue;
pthread_mutex_t lock;
int connections_handled = 0;
int server_running = 1;
threadlist *threads;

int main(int argc, char *argv[])
{
    if (argc < 3){
	fprintf(stderr, "usage: %s portnumber filename\n", argv[0]);
	exit(1);
    }

    // Attempt to open the log file - if invalid, exit.
    open_log_file(argv[2]);
    
    size_t clilen;
    int sockfd, portno;
    t_struct *thread_data;
    struct sockaddr_in serv_addr, cli_addr;
    int result;
    
    printf("Starting server listening on port %s, logging to %s.\n", argv[1], argv[2]);
    
    /*
     * socket(namespace, style, protocol)
     * namespace must be PF_LOCAL or PF_INET
     * style is the style of communication. Stream give you a pipe, for dgram you 
     * must specify recipient address each time.
     * protocol designates a specific protocol (zero is usually OK)
     */
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    
    if (sockfd < 0){
	free(fp);
	error("Error opening socket.");
    }
    

    printf("Socket opened successfully.\n");
    
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
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
	free(fp);
	error("Error on binding");
    }
    
    printf("Socket bound successfully.\n");


    /* 
     * Register the signal handler.
     */
    struct sigaction handler;
    handler.sa_handler = sig_handler;
    handler.sa_flags = 0;
    sigfillset(&handler.sa_mask);
    
    check_handler_setup(sigaction(SIGINT, &handler, NULL), "Failed to set up SIGINT handler");
    check_handler_setup(sigaction(SIGTERM, &handler, NULL), "Failed to set up SIGTERM handler");
        
    /* enables the socket to accept connections - the second argument is the length of the queue. */
    listen(sockfd, 5);
    printf("Listening for connections...\n");
    clilen = sizeof(cli_addr);
    
    while(1){
	// Create a struct in which to store the socket reference and a termination
	// request flag.
	thread_data = malloc(sizeof(t_struct));
			    
	if (!thread_data)
	    fprintf(stderr, "Failed to allocate memory for new thread struct.\n");
	
	
	if (length(threads) == MAX_THREADS){
	    printf("All threads are currently occupied. Waiting for one to become free.\n");
	} else {
	    printf("%d of %d threads are currently occupied.\n", length(threads), MAX_THREADS);
	}
	while (length(threads) == MAX_THREADS){ // Wait for a thread to become available.
	    if (!server_running)
		break;
	    sleep(4);
	    threads = prune_terminated_threads(threads);
	}

	if (!server_running){
	    free(thread_data);
	    break;
	}
	
	printf("Thread space available. Creating...\n");

	/*
	 * accepts a connection request on the server socket.
	 * accept (int socket, struct sockaddr *addr, socklen_t *length_ptr)
	 * addr and length_ptr are used to return information about the 
	 * name of the client socket.
	 * returns a file descriptor for a new socket which is connected to the client.
	 */
	thread_data->socket = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	thread_data->thread_ref = malloc(sizeof(pthread_t));
	thread_data->termreq = 0;
	thread_data->terminated = 0;
			
	if (!server_running){
	    send_term_message(thread_data->socket);
	    close(thread_data->socket);
	    free(thread_data->thread_ref);
	    free(thread_data);
	    break;
	}
	
	if (!thread_data->thread_ref)
	    fprintf(stderr, "Failed to allocate memory for new thread.");

	printf("Allocated thread memory\n");
	
	if (thread_data->socket < 0)
	    error("ERROR on accept");

	
	pthread_attr_t pthread_attr;

	printf("Successfully created handler thread.\n");

	// initialises the thread object with default attributes.
	if (pthread_attr_init(&pthread_attr)){
	    fprintf(stderr, "Creation of thread attributes failed.\n");
	    exit(1);
	}

	printf("Thread attributes initialised.\n");
	
	/*
	 * detached state frees all thread resources when the thread terminates.
	 * in joinable state, thread resources are kept allocated after it terminates,
	 * but it is possible to synchronise on the thread termination and recover
	 * its termination code.
	 * In this case we set the state to detached.
	 */
	if (pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED)){
	    fprintf(stderr, "Setting thread attributes failed.\n");
	    exit(1);
	}

	printf("Set thread state.\n");

	/*n
	 * int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg);
	 * creates a new thread with the attributes specified in attr. Modifying the values of attr later
	 * will not affect thread behaviour. On successful creation, the ID of the thread is returned.
	 * The thread starts executing start_routine with arg as its only argument.
	 */
	printf("Starting thread...\n");
	result = pthread_create(thread_data->thread_ref, &pthread_attr, logstring, (void *) thread_data);
	if (result != 0){
	    fprintf(stderr, "Thread creation failed.\n");
	    exit(1);
	}

	threads = prune_terminated_threads(threads);
	threads = add(threads, thread_data);
	printf("Currently running threads:\n");
	print_list(threads);
	
	connections_handled++;
    }

    return 0;
}

/*
 * logs a string of form <alphanum> : <textchar> to the file specified
 * when the server is started up.
 */
void* logstring(void *args)
{
    t_struct *tdata = (t_struct*) args;
    //char buffer[BUFFERLENGTH];
    //int n;
        
    printf("Thread processing requests on socket %d.\n", tdata->socket);

    /*
     * read (int filedes, void *buffer, size_t size)
     * reads up to size bytes from the file with descriptor filedes into buffer.
     * returns the number of bytes actually read. zero return indicates EOF.
     */
    char *message = NULL;
    while(1){

	
	// If the server has asked the thread to terminate, send a termination message and
	// break. Note that the client will receive the termination message on the next message
	// it sends, not the current one.
	if (tdata->termreq == 1){
	    printf("Termination request received.\n");
	    //send_term_message();
	    break;
	}
	
	message = receive_message(tdata->socket);
	if (strcmp(message, "EOF") == 0){
	    break;
	}

	int v = valid_string(message);
	send_message_valid(v, tdata->socket);

	printf("Message %s was %s.\n", message, v == 0 ? "invalid" : "valid");

	if (v != 0){
	    write_to_file(message);
	}
	    
	free(message);
	printf("\n");
	
    }
    
    printf("Client disconnected, thread for socket %d exited.\n", tdata->socket);
    // close the socket and free the memory.
    close(tdata->socket);
    tdata->terminated = 1;
        
    // return some value as the exit status of the thread.
    returnValue = 0;
    pthread_exit(&returnValue);
}



/*
 * Checks whether the given string is a valid log string. Returns 1 if true, and 
 * 0 otherwise.
 */
int valid_string(char *str)
{
    int c_flag = 0; // whether we reached the colon or not.

    for (; *str != '\0'; str++){
	if (*str == ':'){
	    if (*(str + 1) == '\0')
		break;
	    else
		c_flag = 1;
	}
		
	// Before the colon is reached, check that characters are alphanumeric.
	if (!isalnum(*str) && !c_flag){
	    printf("'%c' is not alphanumeric\n", *str);
	    return 0;
	}
	
	// check characters after the colon are in the valid range.
	if (c_flag && (*str < 32 || *str > 126)){
	    printf("'%c' is not in the valid range\n", *str);
	    return 0;
	}
    }

    // if there was no colon, the string is invalid.
    if (!c_flag)
	return 0;
    
    return 1;
}

// Opens the file that we will log to.
void open_log_file(char *filename)
{
    if ((fp = fopen(filename, "a")) == NULL){
	error("Could not open the specified file for logging.");
    }
}

/*
 * Writes a character string to the globally specified file, which should already be open.
 * If the write fails for whatever reason, 0 is returned. Otherwise, 1 is returned.
 */
int write_to_file(char *str)
{
    int ok;
    
    printf("Logging %s to file.\n", str);
    
    pthread_mutex_lock(&lock);

    ok = fprintf(fp, "%s\n", str);
    fflush(fp);
    
    pthread_mutex_unlock(&lock);
    
    if (ok < 0)
	return 0;
    return 1;
}

/*
 * Makes a termination request to the thread with the specified socket
 * number. If socket_number is negative, all threads will be asked to 
 * terminate.
 */
void request_thread_termination(int socket_number)
{
    set_terminate_request(threads, socket_number);
}


/*
 * Shuts the server down gracefully. Will wait for threads to finish receiving
 * the current message from the client and then close the connection and log file.
 */
void shutdown_server()
{
    request_thread_termination(-1);
    printf("Waiting on threads to exit...\n");
    while (length(threads) != 0){
	threads = prune_terminated_threads(threads);
	sleep(2);
    }
    
    free_list(threads);
    
    fclose(fp); // only close the file once the threads are done.

    printf("Shutdown complete.\n");
}

/*
 * Sets the server to shut down, but will wait for all threads to exit without
 * ordering them to close. The server will no longer accept new connections, but 
 * will wait for all clients to disconnect.
 */
/* void soft_shutdown() */
/* { */
/*     printf("Waiting on all clients to disconnect...\n"); */
/*     while (length(threads) != 0) { */
	
/* 	threads = prune_terminated_threads(threads); */
/*     } */
    
/* } */

// Handles signals. Will shut down the server on receipt. Once one signal has been received,
// Things will break if SIGINT or SIGTERM are received again.
void sig_handler(int signo)
{
    if (server_running){
	printf("\n Received term signal - shutting down server...\n");
	printf("Total number of connections handled: %d\n", connections_handled);
	server_running = 0;
	shutdown_server();
	//soft_shutdown();
    }
    
}
