#include "limiter.h"

void check(int result, char *err_msg)
{
    if (result != 0) {
	printf("%s\n", err_msg);
    }
    
}
