#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <linux/types.h>
#include <linux/netfilter.h>		/* for NF_ACCEPT */

#include <libnetfilter_queue/libnetfilter_queue.h>
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>

void check(int result, char* err_msg);

void print_pkt (struct nfq_data *tb);
u_int32_t get_pkt_id(struct nfq_data *tb);
unsigned short get_src_port(struct nfq_data *nfa);
unsigned short get_dest_port(struct nfq_data *nfa);
