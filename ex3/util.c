#include "limiter.h"

void check(int result, char *err_msg)
{
    if (result != 0) {
	printf("%s\n", err_msg);
	exit(1);
    }
    
}

void print_pkt (struct nfq_data *tb)
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

}

u_int32_t get_pkt_id(struct nfq_data *tb)
{
    int id = 0;
    struct nfqnl_msg_packet_hdr *ph;
 
    ph = nfq_get_msg_packet_hdr(tb);
    
    if (ph) {
	id = ntohl(ph->packet_id);
	//printf("hw_protocol=0x%04x hook=%u id=%u ",
	//       ntohs(ph->hw_protocol), ph->hook, id);
    }

    return id;
    
}

unsigned short get_src_port(struct nfq_data *nfa) 
{
    char* buffer;
        
    nfq_get_payload(nfa, &buffer);
    
    return *((unsigned short*) (buffer + 22));
}

unsigned short get_dest_port(struct nfq_data *nfa)
{
    char *buffer;
        
    nfq_get_payload(nfa, &buffer);

    return  *((unsigned short*) (buffer + 20));
}
