#include "ex1.h"

/* int main(int argc, char *argv[]) */
/* { */
/*     strlist *list = NULL; */
    
/*     list = insert_ordered(list, "ab"); */
/*     list = insert_ordered(list, "ag"); */
/*     list = insert_ordered(list, "."); */
/*     list = insert_ordered(list, ".."); */
/*     list = insert_ordered(list, "01234"); */
/*     list = insert_ordered(list, "012a4"); */
    
    
/*     print_list(list); */
/*     printf("%d\n", length(list)); */
/*     free_list(list); */
        
/*     return 0; */
/* } */

/* THIS FUNCTION SHOULD ONLY BE CALLED FROM FUNCTIONS IN THIS FILE.
   initialises the list and returns a pointer to the start of the list */
strlist* init_list(char *str)
{
    strlist *head = malloc(sizeof(strlist));
    head->s = str;
    head->next = NULL;
        
    return head;
}

/* Adds a string into the list. */
strlist* add(strlist *head, char *str)
{
    strlist *new = malloc(sizeof(strlist));
    new->s = str;
    new->next = head;

    return new;
}

/* Inserts a string into the list in an ordered fashion. The head
   of the list is the string with the highest alphanumeric value.
*/
strlist* insert_ordered(strlist *head, char *str)
{

    /* If head is not yet initialised, put the string in and
       return head.
    */
    if (head == NULL)
	return init_list(str);

    // Only allocate memory after being sure that it is needed.
    strlist *new = malloc(sizeof(strlist));

    /* If the string you want to insert is greater than the one
       currently in head, make a new strlist to replace head.
    */
    if (strcomp(str, head->s) == 1){
	new->s = str;
	new->next = head;
	return new;
    }
    
    strlist *prev;
    strlist *cur = head;
    
    /* If the string being inserted is less than the value in head,
       go down the list until you find a lower-value string, then 
       insert in that location, and return the unchanged head 
       of the list. If the values are equal, the string being
       inserted will go before the one previously inserted.
    */
    while (cur != NULL && strcomp(str, cur->s) != 1){
	prev = cur;
	cur = cur->next;
    }
    
    new->s = str;
    new->next = cur;
    prev->next = new;
    
    return head;
}

/* Print the given list to stdout. */
void print_list(strlist *head)
{
    strlist *node = head;
    
    while (node != NULL){
	printf("%s\n", node->s);
	node = node->next;
    }
}

/* Goes through the list, freeing all allocated memory. */
void free_list(strlist *head)
{
    strlist *cur = head;
    strlist *prev;
    
    while (cur != NULL){
	prev = cur;
	cur = cur->next;
	free(prev);
    }
}

/* Returns the length of the list.  */
int length(strlist* head)
{
    int len;

    for (len = 0; head != NULL; ++len, head = head->next);
    
    return len;
}
