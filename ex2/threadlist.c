#include "logserver.h"

/* int main(int argc, char *argv[]) */
/* { */
/*     t_struct *n = malloc(sizeof(t_struct)); */
    
/*     n->socket = 1; */
/*     n->termreq = 0; */
/*     t_struct *n2 = malloc(sizeof(t_struct)); */
    
/*     n2->socket = 2; */
/*     n2->termreq = 0; */

/*     t_struct *n3 = malloc(sizeof(t_struct)); */
    
/*     n3->socket = 3; */
/*     n3->termreq = 0; */

/*     t_struct *n4 = malloc(sizeof(t_struct)); */
    
/*     n4->socket = 4; */
/*     n4->termreq = 0; */

/*     t_struct *n5 = malloc(sizeof(t_struct)); */
    
/*     n5->socket = 5; */
/*     n5->termreq = 0; */

/*     t_struct *n6 = malloc(sizeof(t_struct)); */
    
/*     n6->socket = 6; */
/*     n6->termreq = 0; */

/*     threadlist *tlist = init_list(n); */
/*     tlist = add(tlist, n2); */
/*     tlist = add(tlist, n3); */
/*     tlist = add(tlist, n4); */
    
/*     print_list(tlist); */

/*     printf("deleting.\n"); */

/*     tlist = delete(tlist, 1); */
        
/*     tlist = add(tlist, n5); */

/*     tlist = delete(tlist, 3); */
    
/*     tlist = add(tlist, n6); */
    
/*     print_list(tlist); */
/*     free_list(tlist); */
            
/*     return 0; */
/* } */


/*
 * Initialises a list with the specified t_struct as its head.
 */
threadlist* init_list(t_struct *thread)
{
    threadlist *head = malloc(sizeof(threadlist));
    head->tdata = thread;
    head->next = NULL;
        
    return head;
}

/*
 * Adds a thread to the list.
 */
threadlist* add(threadlist *head, t_struct *thread)
{
    threadlist *new = malloc(sizeof(threadlist));
    
    new->tdata = thread;
    new->next = head;
    
    return new;
}

/*
 * Deletes the list element with the thread struct that contains
 * the given socket number. Returns the list with the specified 
 * element deleted, or the unmodified list if the element is not
 * present.
 */
threadlist* delete(threadlist *head, int socket_number)
{

    threadlist *cur = head;
    threadlist *prev = cur;
        
    while (cur != NULL){
	if (cur->tdata->socket == socket_number){
	    // Set the previous element to skip the one that we will delete
	    prev->next = cur->next;
	    if (cur == head){ // special case if we have to delete the head of the list.
		printf("deletng head\n");
		cur = cur->next;
		free(head->tdata);
		free(head);
		return cur;
	    }
	    free(cur->tdata);
	    free(cur);
	    return head;
	}
	prev = cur;
	cur = cur->next;
    }
    return head;
}


/* Print the given list to stdout. */
void print_list(threadlist *head)
{
    threadlist *node = head;
    
    while (node != NULL){
	printf("Socket: %d, Termreq: %d\n", node->tdata->socket, node->tdata->termreq);
	node = node->next;
    }
}

/* Goes through the list, freeing all allocated memory. */
void free_list(threadlist *head)
{
    threadlist *cur = head;
    threadlist *prev;
    
    while (cur != NULL){
	prev = cur;
	cur = cur->next;
	free(prev->tdata);
	free(prev);
    }
}

/* Returns the length of the list.  */
int length(threadlist* head)
{
    int len;

    for (len = 0; head != NULL; ++len, head = head->next);
    
    return len;
}
