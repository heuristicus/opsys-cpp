#include "limiter.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define FSTR_LEN 100

char* fsetup;
char* freset;

static void sig_handler(int signum);

pid_t q_pid;

int main(int argc, char *argv[])
{
        
    if (geteuid() != 0){
	printf("%s: You must be root to run this program.\n", argv[0]);
	exit(1);
    }
    
    if (argc < 5){
	printf("Missing parameters.\nusage: %s port max_connections_per_sec wait_time firewall_conf firewall_reset\n", argv[0]);
	exit(1);
    }

    struct sigaction handler;
    handler.sa_handler = sig_handler;
    handler.sa_flags = 0;
    sigfillset(&handler.sa_mask);
    
    check(sigaction(SIGINT, &handler, NULL), "Could not bind SIGINT action handler.");
    check(sigaction(SIGTERM, &handler, NULL), "Could not bind SIGTERM action handler.");
    
    if (atoi(argv[1]) == 0 || atoi(argv[2]) == 0 || atoi(argv[3]) == 0){
	printf("port, max_connections_per_sec and wait_time must be integer values.\n");
	printf("usage: %s port max_connections_per_sec wait_time firewall_conf firewall_reset\n", argv[0]);
	exit(1);
    }
	
    char *netq_args[5] = {"netqueue", argv[1], argv[2], argv[3], NULL};

    fsetup = malloc(FSTR_LEN);
    freset = malloc(FSTR_LEN);
    
    sprintf(fsetup, "iptables-restore %s", argv[4]);
    sprintf(freset, "iptables-restore %s", argv[5]);
        
    printf("Applying iptables config from file %s\n", argv[4]);
    system(fsetup); // This is quite insecure...
    

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


static void sig_handler(int signum)
{
    int status;
    
    printf("ratelimit: %s received. Killing queue handler.\n", strsignal(signum));
    kill(q_pid, 15); // Send SIGTERM to the child
    q_pid = wait(&status); // Wait for the child to exit
    printf("Child process %d exited with status %d\n", q_pid, status);
    printf("Resetting iptables\n");
    system(freset);
    printf("Parent exiting.\n");
}
