#include "loggeneral.h"
#include <pthread.h>
#include <signal.h>

void* logstring(void *args);
int valid_string(char *str);
int write_to_file(char *str);
void open_log_file(char *filename);
void close_log_file();
void sig_handler(int signo);

