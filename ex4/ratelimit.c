/*
 * Many functions here are adapted from Eike Ritter's example code,
 * found at http://www.cs.bham.ac.uk/~exr/teaching/lectures/opsys/12_13/examples/kernelProgramming/kernel/
 */

#include <linux/module.h>  /* Needed by all modules */
#include <linux/kernel.h>  /* Needed for KERN_ALERT */
#include <linux/netfilter.h> 
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/compiler.h>
#include <linux/smp_lock.h>
#include <linux/timer.h>
#include <net/tcp.h>

// Define some constants which we will use when we parse data from
// the proc file.
#define SET_PORT 'P' // port number to apply rate limiting to
//#define ADD_PORT 'A'
//#define RMV_PORT 'R'
#define SET_LIMIT 'L' // max conns/sec
#define SET_WAIT 'W' // wait time after limit exceeded
#define BUFFERLENGTH 256

MODULE_AUTHOR ("Michal Staniaszek <mxs968@cs.bham.ac.uk>");
MODULE_DESCRIPTION ("Extensions to the firewall") ;
MODULE_LICENSE("GPL");

int init_fw_hook(void);
int tmr_init(void);
void timerFun (unsigned long arg);

struct nf_hook_ops *reg;
struct timer_list tmr;
struct proc_dir_entry *procKernelRead;
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

// Exports the symbol for dynamic linking. This allows any other kernel module to
// make use of this method. The definitions of printk and so on are made available
// in a similar way.
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
 * Read data from the proc file. Called from user space, so sleeping is ok? You can pass data to
 * this method by writing to the proc file. This can be done in a number of ways. Either you can
 * write a user space program to do it, or you can use something like "echo 'P 2000' > /proc/ratelimit"
 * which can do pretty much the same stuff you can do with a user space program. It's probably easier
 * to use the bash method if you're just passing in some short parameters.
 */
int kernelRead (struct file *file, const char *buffer, unsigned long count, void *data) { 


    char *kernelBuffer; /* the kernel buffer */
 
    // Second argument controls the behaviour of the memory allocation. GFP_KERNEL is the
    // 'default' argument. It is allowed to sleep, so is appropriate for calls from user space
    // GFP_ATOMIC, on the other hand, is not allowed to sleep, and should be used to allocate
    // memory from interrupt handlers and other code outside of process context.
    kernelBuffer = kmalloc(BUFFERLENGTH, GFP_KERNEL);
   
    if (!kernelBuffer) {
	return -ENOMEM;
    }

    if (count > BUFFERLENGTH) { /* make sure we don't get buffer overflow */
	kfree(kernelBuffer);
	return -EFAULT;
    }

    /* copy data from user space */
    if (copy_from_user(kernelBuffer, buffer, count)) { 
	kfree(kernelBuffer);
	return -EFAULT;
    }
      
    kernelBuffer[BUFFERLENGTH -1] = '\0'; /* safety measure: ensure string termination */
    printk(KERN_INFO "kernelRead: Having read %s\n", kernelBuffer);

    switch (kernelBuffer[0]) {
    case SET_PORT:
	printk(KERN_INFO "Setting port.\n");
	break;
    case SET_LIMIT:
	printk(KERN_INFO "Setting limit\n");
	break;
    case SET_WAIT:
	printk(KERN_INFO "Setting wait\n");
	break;
    default:
	printk(KERN_INFO "kernelRead: Illegal command \n");
    }
    return count;
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

    // Creates a virtual file to be used for passing values between kernel space and
    // user space. Permission bits are used to determine who can access the file.
    // More about that at http://www.gnu.org/software/libc/manual/html_node/Permission-Bits.html
    procKernelRead = create_proc_entry ("ratelimit", S_IWUSR | S_IRUGO, NULL);
        
    if (!procKernelRead) {
	return -ENOMEM;
    }
    
    procKernelRead->write_proc = kernelRead;
    
    return errno;
}

/*
 * Called when the module is shut down (usually with rmmod(?)). Should free all
 * allocated memory and perform any necessary cleanup. Anything created within
 * this module which is no longer required by anyone else should be destroyed.
 */
void cleanup_module(void)
{

    // Prevent any more calls to the module
    remove_proc_entry ("ratelimit", NULL);
    
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
