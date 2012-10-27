#include "loggeneral.h"
#include <pthread.h>
#include <signal.h>

#define MAX_THREADS 2

struct t_struct
{
    int socket;
    int termreq;
    pthread_t thread_ref;
    int terminated;
};
typedef struct t_struct t_struct;

struct threadlist
{
    t_struct *tdata;
    struct threadlist *next;
};
typedef struct threadlist threadlist;

threadlist* init_list(t_struct *thread);
threadlist* add(threadlist *head, t_struct *thread);
threadlist* delete(threadlist *head, int socket_number);
void print_list(threadlist *head);
void free_list(threadlist *head);
int length(threadlist *head);
threadlist* prune_terminated_threads(threadlist *head);
int set_terminate_request(threadlist *head, int socket_number);

void* logstring(void *args);
int valid_string(char *str);
int write_to_file(char *str);
void open_log_file(char *filename);
void close_log_file();
void sig_handler(int signo);
void shutdown_server();
int threads_active();
void terminate_threads();
void client_shutdown_handler(int signo);

