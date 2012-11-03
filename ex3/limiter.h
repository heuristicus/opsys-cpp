#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <linux/types.h>
#include <linux/netfilter.h>		/* for NF_ACCEPT */

#include <libnetfilter_queue/libnetfilter_queue.h>
#include <signal.h>

void check(int result, char* err_msg);
