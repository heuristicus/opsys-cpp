#include "loggeneral.h"

// Prints an error message and exits the program.
void error(char *msg)
{
    perror(msg);
    exit(1);
}

/*
 * Returns the integer x represented in a string. max_size
 * is the maximum number of characters in the string.
 */
/* char* get_integer_string(int x, int max_size) */
/* { */
/*     char str[max_size]; */
/*     snprintf(str, max_size, "%d", x); */
/*     return str; */
/* } */
