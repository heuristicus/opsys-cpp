#include "ex1.h"

typedef struct 
{
    char *s;
    struct strlist *next;
    
} strlist;

strlist make_list(char* s);
int length(struct strlist* head);


/* initialises the list and returns a pointer to the start of the list */
struct strlist* init_list(char* str)
{
    strlist head = malloc(sizeof(struct strlist));
    head->s = str;
    head->next

    return head
}
