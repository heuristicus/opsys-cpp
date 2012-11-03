#include "limiter.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>

#include <signal.h>

static void sig_handler(int signum);

pid_t q_pid;

int main(int argc, char *argv[])
{
        
    if (geteuid() != 0){
	printf("%s: You must be root to run this program.\n", argv[0]);
	exit(1);
    }
    
    if (argc < 3){
	printf("Missing parameters.\nusage: %s port max_connections_per_sec\n", argv[0]);
	exit(1);
    }

    struct sigaction handler;
    handler.sa_handler = sig_handler;
    handler.sa_flags = 0;
    sigfillset(&handler.sa_mask);
    
    check(sigaction(SIGINT, &handler, NULL), "Could not bind SIGINT action handler.");
    check(sigaction(SIGTERM, &handler, NULL), "Could not bind SIGTERM action handler.");
    
    char *netq_args[4] = {"netqueue", argv[1], argv[2], NULL};
    
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
    
    printf("ratelimit: SIGINT received. Killing queue handler.\n");
    kill(q_pid, 15); // Send SIGTERM to the child
    q_pid = wait(&status); // Wait for the child to exit
    printf("Child process %d exited with status %d\n", q_pid, status);
    printf("Parent exiting.\n");
}
