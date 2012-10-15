#include "ex1.h"

/* Returns the result of an alphanumeric comparison of string s1 and s2. 
 * If s1 is of greater value than the second, the function returns 1.
 * If they are equal, 0 is returned. If the second is greater, -1 is returned.
 * a has the highest value, z has the lowest. 
 * Characters are converted to lowercase before being compared.
 * Shorter strings are considered higher value.
 * Strings are expected to have a null terminator at the end.
 */
int strcomp(char* s1, char* s2)
{
    if (s1 == NULL && s2 == NULL)
	return 0;
        
    for(; ; ++s1, ++s2){
	if (s1 == NULL || tolower(*s1) > tolower(*s2) || *s2 == '\0')
	    return -1;
	else if (s2 == NULL || tolower(*s1) < tolower(*s2) || *s1 == '\0')
	    return 1;
    }
    
    return 0;
    
}
