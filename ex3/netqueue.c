#include "limiter.h"

//#define VERBOSE

static void sig_handler(int signum);
static void setup_queue();
static void* handle_packets();
static void unbind_queue();
static void setup_timer();
static void alrm_handler(int signum);
static void reset_timer(int interval);

struct nfq_handle *h;
struct nfq_q_handle *qh;
struct itimerval timer;

int conn_num = 0;
int drop_count = 0;
int accept_count = 0;
int elapsed_time = 0;
int limit_exceeded_count = 0;

int limit_exceeded; // If this is positive, we have exceeded the threshold
int accept_packets = 1;

pthread_mutex_t lock;
int *threshold;
int *reject_time;

int main(int argc, char *argv[])
{

    if (geteuid() != 0){
	printf("%s: You must be root to run this program.\n", argv[0]);
	exit(1);
    }
    
    if (argc < 4){
	printf("usage: %s portno max_connections_per_sec wait_time\n", argv[0]);
    }

    // Store values in pointer in case it becomes necessary to use multiple threads. 
    int t = atoi(argv[2]);
    threshold = &t;
    int r = atoi(argv[3]);
    reject_time = &r;
                
    struct sigaction handler;
    handler.sa_handler = sig_handler;
    handler.sa_flags = 0;
    sigfillset(&handler.sa_mask);
    
    check(sigaction(SIGTERM, &handler, NULL), "Failed to assign handler to SIGTERM");
    //sigaction(SIGINT, &handler, NULL);
    
    // Ignore SIGINT signals - the parent will take care of these
    // and send us a sigterm if we should terminate.
    signal(SIGINT, SIG_IGN);

    setup_queue();

    pthread_t *thread = malloc(sizeof(pthread_t));
    pthread_attr_t pthread_attr;

    if (pthread_attr_init(&pthread_attr)){
	fprintf(stderr, "Creation of thread attributes failed.\n");
	exit(1);
    }

    if (pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED)){
	fprintf(stderr, "Setting thread attributes failed.\n");
	exit(1);
    }

    // Run the packet handler in a thread so that we can handle timer interrupts.
    int result = pthread_create(thread, &pthread_attr, handle_packets, NULL);
    if (result != 0){
	fprintf(stderr, "Thread creation failed.\n");
	exit(1);
    }

    while(1){
	// non-busy wait. Keep the program running while the thread runs.
	// so that we can handle the timer interrupts.
	sleep(1); 
    }

    return 0;
}

/*
 * Handles packets. A timer is started to monitor the number of connection
 * attempts per second. Each time a packet is received, a shared variable
 * is incremented, and each time the timer ticks this value is reset. If 
 * the number of connections received exceeds a threshold value, we wait for
 * a time before accepting connections again. i.e. if the threshold is exceeded,
 * all packets will be dropped for a certain time.
 */
static void* handle_packets(void *data)
{
    /*
     * The aligned attribute forces the compiler to ensure that
     * the buf array is aligned to a specific byte boundary. In the
     * case where no argument is provided to ((aligned)), the compiler
     * chooses the best option, but in the case of ((aligned (8))), the
     * start of the memory block will be an address 0x?????8 (i guess?)
     */
    char buf[4096] __attribute__ ((aligned));
    int rv;
    
    int fd = nfq_fd(h);


    setup_timer(); // start the timer which resets the attempt count
    
    /*
     * ssize_t recv(int sockfd, void *buf, size_t len, int flags);
     * 
     * Receives a message from the specified socket. Will wait for
     * a message to arrive unless the socket is nonblocking.
     */
    while ((rv = recv(fd, buf, sizeof(buf), 0)) && rv >= 0) {
    	/*
    	 * Handles a packet received from the nfqueue subsystem.
    	 * The data in buf is passed to the callback, and rv is the
    	 * length of the data received.
    	 */
    	nfq_handle_packet(h, buf, rv);
	
	// If the limit has been exceeded and we are still accepting packets
	if (limit_exceeded > 0 && accept_packets){ 
	    pthread_mutex_lock(&lock);
	    accept_packets = 0; // Drop all packets we receive
	    pthread_mutex_unlock(&lock);
	    
	    limit_exceeded_count++;
	    printf("Over limit. Sending alarm. Will drop all packets for the next %d seconds.\n", *reject_time);
	    /* 
	     * When we do this the timer should be set to expire in the
	     * specified number of seconds. When the alarm goes off, we will
	     * reset the timer to its original values and accept packets again.
	     */
	    alarm(*reject_time); 
	}
    }

    return 0;
}
	
/*
 * Called whenever a packet is received by the socket.
 * Which packets are received depends on the QUEUE parameter being
 * set on a specific socket. The line below queues only syn packets
 * on port 2200. Syn packets are used to request connections in the TCP protocol, so
 * dropping syn packets will prevent tcp connections from forming. Once the syn
 * packet has been accepted, following packets of the same connection will
 * be accepted.
 * -A INPUT -p tcp --dport 2200 --syn -j QUEUE
 *
 * qh - The queue handle returned by nfq_create_queue
 * nfmsg - message objetc that contains the packet
 * nfad - Netlink packet data handle
 * data - the value passed to the data parameter of nfq_create_queue
 */
static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
	      struct nfq_data *nfa, void *data)
{

    u_int32_t id = get_pkt_id(nfa);

    //printf("Received packet from port %d, destined for port %d\n", get_src_port(nfa), get_dest_port(nfa));

    pthread_mutex_lock(&lock);
    
    conn_num++;
    // If this value ever exceeds zero, we are in excess of the limit.
    limit_exceeded = conn_num - *threshold; 
    
    pthread_mutex_unlock(&lock);

#ifdef VERBOSE
    printf("Connections remaining until limit exceeded: %d\n", -limit_exceeded);
#endif

    // If we have not exceeded the limit and packets are being accepted
    if (limit_exceeded <= 0 && accept_packets){ 
#ifdef VERBOSE
	printf("Packet accepted...\n");
#endif
	accept_count++;
	return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
	// the limit has been exceeded or we are not accepting packets
    } else if (limit_exceeded > 0 || !accept_packets){
#ifdef VERBOSE
	printf("Dropping packet...\n");
#endif
	drop_count++;
	return nfq_set_verdict(qh, id, NF_DROP, 0, NULL);
    } else { // this case should never happen
	printf("SOMETHING WENT WRONG!\n");
	return nfq_set_verdict(qh, id, NF_DROP, 0, NULL);
    }
    
}

/*
 * Set up the queue handler that we will bind the queue to, and then
 * bind the queue to it. Once we have bound the queue, set up an
 * exit handler so that the queue is destroyed correctly.
 */
static void setup_queue()
{
    /*
     * Opens an NFQUEUE connection handler. This creates a netlink connection
     * (what?) which is associated with the handler returned.
     */
    printf("opening library handle\n");
    h = nfq_open();
    if (!h) {
	fprintf(stderr, "error during nfq_open()\n");
	exit(1);
    }

    /*
     * Stops the given handle from processing packets belonging to the specified
     * family.
     */
    printf("unbinding existing nf_queue handler for AF_INET (if any)\n");
    if (nfq_unbind_pf(h, AF_INET) < 0) {
	fprintf(stderr, "error during nfq_unbind_pf()\n");
	exit(1);
    }

    /*
     * Bind handler to the given protocol family. The handler will now process
     * packets received with the AF_INET protocol.
     */
    printf("binding nfnetlink_queue as nf_queue handler for AF_INET\n");
    if (nfq_bind_pf(h, AF_INET) < 0) {
	fprintf(stderr, "error during nfq_bind_pf()\n");
	exit(1);
    }

    /*
     * Creates a queue with the given handle. The second parameter, num, must be a
     * value that does not already have another queue bound to it. The final parameter
     * is the data that is sent to the callback function.
     * The queue ***MUST*** be unbound before the program exits - we cannot access it
     * otherwise
     */
    printf("binding this socket to queue.\n");
    qh = nfq_create_queue(h, 0, &cb, NULL);
    if (!qh) {
	fprintf(stderr, "error during nfq_create_queue()\n");
	exit(1);
    }
    
    /*
     *  Whenever the program exits, unbind_queue must be called to ensure that 
     * destroy the queue. We set this here so that it only happens if the queue
     * has actually been created. If this is being used, do not call unbind queue
     * in the signal handler.
     */
    atexit(unbind_queue); 
    
    /*
     * Sets the amount of data that is copied to user space for each packet which
     * enters the queue. 
     *
     * NFQNL_COPY_NONE - do not copy any data
     * NFQNL_COPY_META - copy only packet metadata
     * NFQNL_COPY_PACKET - copy entire packet
     */
    printf("setting copy_packet mode\n");
    if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
	fprintf(stderr, "can't set packet_copy mode\n");
	exit(1);
    }

}

/*
 * Destroys the queue that we created to handle packets.
 */
static void unbind_queue()
{
    sigset_t sigset;
    sigfillset(&sigset); 
    // Block all signals so that we are guaranteed to destroy the queue.
    sigprocmask(SIG_BLOCK, &sigset, NULL);
        
    // Destroy the queue handle. This also unbinds the handler.
    printf("unbinding from queue\n");
    nfq_destroy_queue(qh);

#ifdef INSANE
    /* normally, applications SHOULD NOT issue this command, since
     * it detaches other programs/sockets from AF_INET, too ! */
    printf("unbinding from AF_INET\n");
    nfq_unbind_pf(h, AF_INET);
#endif

    printf("closing library handle\n");
    nfq_close(h);

    // Unblock signals.
    sigprocmask(SIG_UNBLOCK, &sigset, NULL);
}

/*
 * Sets up a timer which we use to trigger an interrupt which resets
 * the connection attempt count.
 */
static void setup_timer()
{
    struct sigaction alrm;
    alrm.sa_handler = alrm_handler;
    alrm.sa_flags = 0;
    sigfillset(&alrm.sa_mask);
    
    check(sigaction(SIGALRM, &alrm, NULL), "Failed to set up alarm signal handler");
        
    timer.it_value.tv_sec = 1;
    timer.it_value.tv_usec = 0;
    
    timer.it_interval = timer.it_value;

    check(setitimer(ITIMER_REAL, &timer, NULL), "Failed to set up timer.");
        
}

/*
 * Handler for the timer. Resets the shared variable containing connection
 * attempt count.
 */
static void alrm_handler(int signum)
{
    if (accept_packets){ // If we are currently accepting packets.
	printf("Handled %d connections in the last second.\n", conn_num);

	elapsed_time++;
        
	pthread_mutex_lock(&lock);
    
	conn_num = 0;
    
	pthread_mutex_unlock(&lock);
    } else { 
	// We are not currently accepting packets - this should only be entered
	// after the alarm we set has expired. When this happens we reset the
	// timer to continue normal operation and set packets to be accepted again.
	
	printf("Dropped %d connection attempts while not accepting connections.\n", conn_num);

	pthread_mutex_lock(&lock);
	
	conn_num = 0;
	accept_packets = 1;
		
	pthread_mutex_unlock(&lock);
	
	reset_timer(1);
    }
    
    
}

/*
 * Resets the timer to tick on the specified interval.
 */
static void reset_timer(int interval)
{
    timer.it_value.tv_sec = interval;
    timer.it_interval = timer.it_value;
    
    setitimer(ITIMER_REAL, &timer, NULL);
}

/*
 * Handles sigterm iterrupts. The rate limiter will send SIGTERM when it
 * receives a SIGINT or somesuch interrupt.
 */
static void sig_handler(int signum)
{
    if (signum == SIGTERM){
	printf("netqueue:got SIGTERM \n");
	printf("Handled %d connections in total. "\
	       "\nAccepted %d, dropped %d packets.\n"\
	       "The threshold was exceeded %d times.\n", accept_count + drop_count, accept_count, drop_count, limit_exceeded_count);
	exit(1);
    } else {
	perror("Error.");
    }
    
}
