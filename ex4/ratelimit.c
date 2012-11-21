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

struct nf_hook_ops *reg;
struct timer_list myTimer;
int i;

unsigned int FirewallExtensionHook (unsigned int hooknum,
				    struct sk_buff *skb,
				    const struct net_device *in,
				    const struct net_device *out,

				    int (*okfn)(struct sk_buff *)) {

    struct tcphdr *tcp;
    struct tcphdr _tcph;

    /* get the tcp-header for the packet */
    tcp = skb_header_pointer(skb, ip_hdrlen(skb), sizeof(struct tcphdr), &_tcph);
    if (!tcp) {
	printk (KERN_INFO "Could not get tcp-header!\n");
	return NF_ACCEPT;
    }
    if (tcp->syn) {
	struct iphdr *ip;
	
	printk (KERN_INFO "firewall: Starting connection \n");
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
    }
    return NF_ACCEPT;	
}

EXPORT_SYMBOL (FirewallExtensionHook);

void timerFun (unsigned long arg) {
    int tmp;
    /* In this simple example, locking is an overkill - this operation should finish within a second  - used here to demonstrate how it works */
    i++;
    tmp = i;
    printk (KERN_INFO "Called timer %d times\n", tmp); 
    myTimer.expires = jiffies + HZ;
    add_timer (&myTimer); /* setup the timer again */
}

int init_module(void)
{
    int errno;
    unsigned long currentTime = jiffies; 
    unsigned long expiryTime = currentTime + HZ; /* HZ gives number of ticks per second */

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
    
    /* pre-defined kernel variable jiffies gives current value of ticks */
    
    init_timer(&myTimer);
    myTimer.function = timerFun;
    myTimer.expires = expiryTime;
    myTimer.data = 0;

    add_timer(&myTimer);
    printk (KERN_INFO "timer added \n");
  
    if (errno) {
	printk (KERN_INFO "Firewall extension could not be registered!\n");
	kfree(reg);
    } else {
	printk(KERN_INFO "Firewall extensions module loaded\n");
    }

    // A non 0 return means init_module failed; module can't be loaded.
    return errno;
}


void cleanup_module(void)
{
    nf_unregister_hook(reg); /* restore everything to normal */
    kfree(reg);
    if (!del_timer (&myTimer)) {
	printk (KERN_INFO "Couldn't remove timer!!\n");
    }
    else {
	printk (KERN_INFO "timer removed \n");
    }
    printk(KERN_INFO "Firewall extensions module unloaded\n");
}
