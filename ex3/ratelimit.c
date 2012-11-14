#include "limiter.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define FSTR_LEN 100

static void sig_handler(int signum);
static void setup_iptables();
static void setup_iptables_file(char *filename);

pid_t q_pid;
int port;
int mode;
char *fsetup;
char *frestore;

int main(int argc, char *argv[])
{
        
    if (geteuid() != 0){
	printf("%s: You must be root to run this program.\n", argv[0]);
	exit(1);
    }

    // 6 params required to read iptables setup from file.
    // Note that there is a huge security hole if you read from files
    if (argc != 6 && argc != 4){
	printf("Missing parameters.\nusage: %s port max_connections_per_sec wait_time {iptables_setup iptables_original}\n", argv[0]);
    	exit(1);
    }

    if (argc == 4)
	mode = 0; // read port mode
    else if (argc == 6)
	mode = 1; // file read mode
    else
	exit(1);

    fsetup = argv[4];
    frestore = argv[5];
        
    struct sigaction handler;
    handler.sa_handler = sig_handler;
    handler.sa_flags = 0;
    sigfillset(&handler.sa_mask);
    
    check(sigaction(SIGINT, &handler, NULL), "Could not bind SIGINT action handler.");
    check(sigaction(SIGTERM, &handler, NULL), "Could not bind SIGTERM action handler.");
    
    // Basic sanity checks
    if ((port = atoi(argv[1])) == 0 || atoi(argv[2]) == 0 || atoi(argv[3]) == 0){
	printf("port, max_connections_per_sec and wait_time must be integer values.\n");
	printf("usage: %s port max_connections_per_sec wait_time\n", argv[0]);
	exit(1);
    }

    char *netq_args[5] = {"netqueue", argv[1], argv[2], argv[3], NULL};

    if (mode == 0)
	setup_iptables("-A");
    else if (mode == 1)
	setup_iptables_file(fsetup);
            
    if ((q_pid = fork()) < 0){ // Fork so that we can execute netqueue
	perror("Something went wrong when attempting to fork.");
	exit(1);
    }
    
    if (q_pid == 0){
	printf("Executing netqueue...\n");
	execv("netqueue", netq_args);
    }
    
    if (q_pid > 0){
	printf("Spawned child process with pid %d\n", q_pid);
	int status;
	// Wait while the child runs.
	q_pid = wait(&status);
    }
    

    return 0;
}

/*
 * Sets up or resets iptables with the port number received in the arguments. The action
 * is -D delete, or -A add. Three rules are added to reroute packets from the kernel into
 * user space.
 */
static void setup_iptables(char* action)
{
    // Basic error check. Only allow commands that we specify.
    if (strcmp("-D", action) && strcmp("-A", action)){
	printf("Invalid action for iptables. %s\n", action);
	exit(1);
    }

    char *scall = malloc(FSTR_LEN);
    // Possible security issues since we are receiving the port number from commandline?
    // using atoi should at least mean that we are getting an integer and not a string, so
    // it is probably safe? This should at least be safer than reading in a filename and
    // then using iptables-restore to read the file (or the malicious code)
    // This sets up the queue to reroute packets that arrive at the specified port.
    sprintf(scall, "iptables %s INPUT -p tcp --dport %d --syn -j QUEUE", action, port);
    system(scall);
    sprintf(scall, "iptables %s INPUT -p tcp --dport %d -m state --state ESTABLISHED,RELATED -j ACCEPT", action, port);
    system(scall);
    sprintf(scall, "iptables %s INPUT -p tcp --dport %d -j DROP", action, port);
    system(scall);
    free(scall);
}

/*
 * Set up iptables config by calling iptables-restore on the given file.
 * Note the security issue here. If the user wants to do something malicious,
 * then the probably can. If they managed to get this far, though, they have
 * super user access, so does it really matter?
 */
static void setup_iptables_file(char *filename)
{
    char *syscall = malloc(strlen(filename) + strlen("iptables-restore ") + 1);
    
    sprintf(syscall, "iptables-restore %s", filename);
    
    system(syscall); // Hack me! (super easy to break)
    
}

static void sig_handler(int signum)
{
    int status;
    
    printf("ratelimit: %s received. Killing queue handler.\n", strsignal(signum));
    kill(q_pid, 15); // Send SIGTERM to the child
    q_pid = wait(&status); // Wait for the child to exit
    printf("Child process %d exited with status %d\n", q_pid, status);
    printf("Resetting iptables\n");
    if (mode == 0)
	setup_iptables("-D");
    else if (mode == 1)
	setup_iptables_file(frestore);
    printf("Parent exiting.\n");
}
