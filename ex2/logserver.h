#include "loggeneral.h"
#include <pthread.h>
#include <signal.h>

#define MAX_THREADS 20

struct t_struct
{
    int socket;
    int shutdown_request;
};
typedef struct t_struct t_struct;

void* logstring(void *args);
int valid_string(char *str);
int write_to_file(char *str);
void open_log_file(char *filename);
void close_log_file();
void sig_handler(int signo);
