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

pid_t q_pid;
int port;

int main(int argc, char *argv[])
{
        
    if (geteuid() != 0){
	printf("%s: You must be root to run this program.\n", argv[0]);
	exit(1);
    }
    
    if (argc < 4){
	printf("Missing parameters.\nusage: %s port max_connections_per_sec wait_time\n", argv[0]);
	exit(1);
    }

    struct sigaction handler;
    handler.sa_handler = sig_handler;
    handler.sa_flags = 0;
    sigfillset(&handler.sa_mask);
    
    check(sigaction(SIGINT, &handler, NULL), "Could not bind SIGINT action handler.");
    check(sigaction(SIGTERM, &handler, NULL), "Could not bind SIGTERM action handler.");
    
    if ((port = atoi(argv[1])) == 0 || atoi(argv[2]) == 0 || atoi(argv[3]) == 0){
	printf("port, max_connections_per_sec and wait_time must be integer values.\n");
	printf("usage: %s port max_connections_per_sec wait_time firewall_conf firewall_reset\n", argv[0]);
	exit(1);
    }

    char *netq_args[5] = {"netqueue", argv[1], argv[2], argv[3], NULL};

    setup_iptables("-A");
        
    if ((q_pid = fork()) < 0){
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
    sprintf(scall, "iptables %s INPUT -p tcp --dport %d --syn -j QUEUE", action, port);
    system(scall);
    sprintf(scall, "iptables %s INPUT -p tcp --dport %d -m state --state ESTABLISHED,RELATED -j ACCEPT", action, port);
    system(scall);
    sprintf(scall, "iptables %s INPUT -p tcp --dport %d -j DROP", action, port);
    system(scall);
    free(scall);
}

static void sig_handler(int signum)
{
    int status;
    
    printf("ratelimit: %s received. Killing queue handler.\n", strsignal(signum));
    kill(q_pid, 15); // Send SIGTERM to the child
    q_pid = wait(&status); // Wait for the child to exit
    printf("Child process %d exited with status %d\n", q_pid, status);
    printf("Resetting iptables\n");
    setup_iptables("-D");
    printf("Parent exiting.\n");
}
