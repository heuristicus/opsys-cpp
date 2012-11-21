#include <linux/module.h>  /* Needed by all modules */
#include <linux/kernel.h>  /* Needed for KERN_ALERT */
#include <linux/netfilter.h> 
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/compiler.h>
#include <linux/smp_lock.h>
#include <linux/timer.h>
#include <net/tcp.h>

MODULE_AUTHOR ("Michal Staniaszek <mxs968@cs.bham.ac.uk>");
MODULE_DESCRIPTION ("Extensions to the firewall") ;
MODULE_LICENSE("GPL");

int init_fw_hook(void);
int tmr_init(void);
void timerFun (unsigned long arg);

struct nf_hook_ops *reg;
struct timer_list tmr;
int pkt_count;
int timer_count;
int alarm_flag;
int active = 1;

spinlock_t my_lock = SPIN_LOCK_UNLOCKED;

/*
 * Called whenever a packet is received on the network. May be called
 * multiple times in very quick succession, from inside the kernel.
 */
unsigned int FirewallExtensionHook (unsigned int hooknum,
				    struct sk_buff *skb,
				    const struct net_device *in,
				    const struct net_device *out,

				    int (*okfn)(struct sk_buff *)) {

    struct tcphdr *tcp;
    struct tcphdr _tcph;
    struct iphdr *ip;
    
    /* get the tcp-header for the packet */
    tcp = skb_header_pointer(skb, ip_hdrlen(skb), sizeof(struct tcphdr), &_tcph);
    if (!tcp) {
	printk(KERN_INFO "Could not get tcp-header!\n");
	return NF_ACCEPT;
    }
    if (tcp->syn) {
	pkt_count++;

	if (pkt_count % 20 == 0)
	    alarm_flag = 1;
		
	if (active){
	    printk(KERN_INFO "firewall: Packet received \n");
	    ip = ip_hdr(skb);
	    if (!ip) {
		printk(KERN_INFO "firewall: Cannot get IP header!\n!");
	    }
	    else {
		printk(KERN_INFO "firewall: Source address = %u.%u.%u.%u\n", NIPQUAD(ip->saddr));
	    }
	    printk(KERN_INFO "firewall: destination port = %d\n", htons(tcp->dest)); 
	    if (htons(tcp->dest) == 80) {
		return NF_DROP;
	    }
	} else {
	    return NF_DROP;
	}
    }
    return NF_ACCEPT;
}

EXPORT_SYMBOL (FirewallExtensionHook);

/*
 * Initialises the timer we will use to reset the connection count.
 */
int tmr_init(void)
{
    unsigned long currentTime = jiffies; 
    unsigned long expiryTime = currentTime + HZ; /* HZ gives number of ticks per second */
    
    init_timer(&tmr);
    tmr.function = timerFun;
    tmr.expires = expiryTime;
    tmr.data = 0;

    add_timer(&tmr);
    printk (KERN_INFO "timer added \n");
    return 0;
}

/*
 * Initialises the firewall hook which is called by another module whenever a
 * packet is received from the network.
 */
int init_fw_hook(void)
{
    int errno;
    
    /* allocate space for hook */
    reg = kmalloc (sizeof (struct nf_hook_ops), GFP_KERNEL);
    if (!reg) {
	return -ENOMEM;
    }

    /* fill it with the right data */
    reg->hook = FirewallExtensionHook; /* the procedure to be called */
    reg->pf = PF_INET;
    reg->owner = THIS_MODULE;
    reg->hooknum = NF_INET_LOCAL_IN; /* only for incoming connections */

    errno = nf_register_hook(reg); /* register the hook */
    
    return errno;
}

/*
 * Called whenever the timer expires. This is called from an interrupt.
 */
void timerFun (unsigned long arg) {
    timer_count++;
    printk(KERN_INFO "Packet count: %d", pkt_count);
    if (alarm_flag){
	printk(KERN_INFO "Alarm flag set. Using extended expiry time.");
	tmr.expires = jiffies + HZ * 5;
	alarm_flag = 0;
    } else {
	printk(KERN_INFO "Using standard timer expiry time.");
	tmr.expires = jiffies + HZ;
    }
    add_timer(&tmr); /* setup the timer again */
}

/*
 * Called when the module is started up (usually with insmod(?)). Should perform
 * module setup, and decide whether the module should be allowed to run or not.
 * A non-zero return indicates that something is wrong and the module should
 * not be started up.
 */
int init_module(void)
{
    int errno = 0;
    int ret;
        
    if ((ret = init_fw_hook())){
	printk (KERN_INFO "Firewall extension could not be registered!\n");
	kfree(reg);
	errno++;
    } else {
	printk (KERN_INFO "Firewall extension registered.\n");
    }
    
    if ((ret = tmr_init())){
	printk (KERN_INFO "Could not initialise timer.\n");
	errno++;
    } else {
	printk (KERN_INFO "Timer initialised.\n");
    }
        
    return errno;
}

/*
 * Called when the module is shut down (usually with rmmod(?)). Should free all
 * allocated memory and perform any necessary cleanup. Anything created within
 * this module which is no longer required by anyone else should be destroyed.
 */
void cleanup_module(void)
{
    // Remove the firewall extension hook
    nf_unregister_hook(reg);
    kfree(reg);
    
    // If the timer is still running, remove it.
    if (!del_timer (&tmr)) {
	printk (KERN_INFO "Couldn't remove timer!!\n");
    }
    else {
	printk (KERN_INFO "Timer successfully removed.\n");
    }
    printk(KERN_INFO "Firewall extensions module unloaded\n");
}
