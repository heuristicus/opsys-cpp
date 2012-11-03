#include "limiter.h"

static void sig_handler(int signum);
static void setup_queue();
static void handle_packets();
static void unbind_queue();

struct nfq_handle *h;
struct nfq_q_handle *qh;

int main(int argc, char *argv[])
{

    if (geteuid() != 0){
	printf("%s: You must be root to run this program.\n", argv[0]);
	exit(1);
    }
    
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
    handle_packets();
        
    return 0;
}

/* returns packet id */
static u_int32_t print_pkt (struct nfq_data *tb)
{
	int id = 0;
	struct nfqnl_msg_packet_hdr *ph;
	struct nfqnl_msg_packet_hw *hwph;
	u_int32_t mark,ifi; 
	int ret;
	char *data;

	ph = nfq_get_msg_packet_hdr(tb);
	if (ph) {
		id = ntohl(ph->packet_id);
		printf("hw_protocol=0x%04x hook=%u id=%u ",
			ntohs(ph->hw_protocol), ph->hook, id);
	}

	hwph = nfq_get_packet_hw(tb);
	if (hwph) {
		int i, hlen = ntohs(hwph->hw_addrlen);

		printf("hw_src_addr=");
		for (i = 0; i < hlen-1; i++)
			printf("%02x:", hwph->hw_addr[i]);
		printf("%02x ", hwph->hw_addr[hlen-1]);
	}

	mark = nfq_get_nfmark(tb);
	if (mark)
		printf("mark=%u ", mark);

	ifi = nfq_get_indev(tb);
	if (ifi)
		printf("indev=%u ", ifi);

	ifi = nfq_get_outdev(tb);
	if (ifi)
		printf("outdev=%u ", ifi);
	ifi = nfq_get_physindev(tb);
	if (ifi)
		printf("physindev=%u ", ifi);

	ifi = nfq_get_physoutdev(tb);
	if (ifi)
		printf("physoutdev=%u ", ifi);

	ret = nfq_get_payload(tb, &data);
	if (ret >= 0)
		printf("payload_len=%d ", ret);

	fputc('\n', stdout);

	return id;
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
	u_int32_t id = print_pkt(nfa);
	printf("entering callback\n");
	return nfq_set_verdict(qh, id, NF_DROP, 0, NULL);
}

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
    qh = nfq_create_queue(h, 4, &cb, NULL);
    if (!qh) {
	fprintf(stderr, "error during nfq_create_queue()\n");
	exit(1);
    }
    printf("Socket bound to queue %d.\n", qh->id);
    	
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
	unbind_queue(); // make sure that the queue gets unbound - otherwise we can never unbind it.
	exit(1);
    }

}

static void handle_packets()
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

    /*
     * ssize_t recv(int sockfd, void *buf, size_t len, int flags);
     * 
     * Receives a message from the specified socket. Will wait for
     * a message to arrive unless the socket is nonblocking.
     */
    while ((rv = recv(fd, buf, sizeof(buf), 0)) && rv >= 0) {
	printf("pkt received\n");
	/*
	 * Handles a packet received from the nfqueue subsystem.
	 * The data in buf is passed to the callback, and rv is the
	 * length of the data received.
	 */
	nfq_handle_packet(h, buf, rv);
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
    printf("unbinding from queue %d\n", qh->id);
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



static void sig_handler(int signum)
{
    printf("netqueue:got SIGTERM \n");
    unbind_queue();
    exit(1);
}
