#include "loggeneral.h"

// Prints an error message and exits the program.
void error(char *msg)
{
    perror(msg);
    exit(1);
}
