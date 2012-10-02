#include "ex1.h"


int main(int argc, char *argv[])
{
    
    return 0;
}

/* initialises the list and returns a pointer to the start of the list */
strlist* init_list(char* str)
{
    strlist *head = malloc(sizeof(strlist));
    head->s = *str;
    head->next = NULL;
    
    return head;
}

int insert(strlist *head, char *str)
{
    return 0;
}

int length(strlist* head)
{
    return 0;
}
