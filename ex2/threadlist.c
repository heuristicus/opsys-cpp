#include "logserver.h"

static threadlist* delete_this_element(threadlist *to_delete, threadlist *previous);

/* int main(int argc, char *argv[]) */
/* { */
/*     t_struct *n = malloc(sizeof(t_struct)); */
    
/*     n->socket = 1; */
/*     n->termreq = 0; */
/*     n->terminated = 1; */
    
/*     t_struct *n2 = malloc(sizeof(t_struct)); */
    
/*     n2->socket = 2; */
/*     n2->termreq = 0; */
/*     n2->terminated = 0; */

/*     t_struct *n3 = malloc(sizeof(t_struct)); */
    
/*     n3->socket = 3; */
/*     n3->termreq = 0; */
/*     n3->terminated = 1; */

/*     t_struct *n4 = malloc(sizeof(t_struct)); */
    
/*     n4->socket = 4; */
/*     n4->termreq = 0; */
/*     n4->terminated = 0; */

/*     t_struct *n5 = malloc(sizeof(t_struct)); */
    
/*     n5->socket = 5; */
/*     n5->termreq = 0; */
/*     n5->terminated = 1; */

/*     t_struct *n6 = malloc(sizeof(t_struct)); */
    
/*     n6->socket = 6; */
/*     n6->termreq = 0; */
/*     n6->terminated = 1; */

/*     threadlist *tlist = init_list(n); */
/*     tlist = add(tlist, n2); */
/*     tlist = add(tlist, n3); */
/*     tlist = add(tlist, n4); */
    
/*     print_list(tlist); */

/*     printf("deleting.\n"); */

/*     //tlist = delete(tlist, 1); */
        
/*     tlist = add(tlist, n5); */

/*     //tlist = delete(tlist, 3); */
    
/*     tlist = add(tlist, n6); */
    
/*     set_terminate_request(tlist, -1); */
/*     printf("setting term flag on all\n"); */
/*     print_list(tlist); */
    
/*     printf("deleting 1\n"); */
/*     tlist = delete(tlist, 1); */
/*     print_list(tlist); */
/*     printf("deleting 6\n"); */
/*     tlist = delete(tlist, 6); */
/*     print_list(tlist); */
/*     tlist = prune_terminated_threads(tlist); */
/*     printf("Pruned.\n"); */
    
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
    if (head == NULL)
	return init_list(thread);
        
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
	    if (cur == head){
		// We delete the head, so set the head to the element that came after it.
		head = delete_this_element(cur, prev);
		break;
	    } else {
		// Delete the current element and exit.
		delete_this_element(cur, prev);
		break;
	    }
	}
	prev = cur;
	cur = cur->next;
    }
    return head;
}

/*
 * Deletes the specified element from the list, linking the element before the 
 * deleted element to the one that comes after it. The element returned is the one
 * that the deleted element was pointing to.
 */
static threadlist* delete_this_element(threadlist *to_delete, threadlist *previous)
{
    threadlist* next_e = to_delete->next;
    previous->next = next_e;
    
    free(to_delete->tdata->thread_ref);
    free(to_delete->tdata);
    free(to_delete);
    
    return next_e;
    
}


/*
 * Removes threads that have been terminated from the list.
 */
threadlist* prune_terminated_threads(threadlist *head)
{
    
    threadlist *cur = head;
    threadlist *prev = cur;
        
    while (cur != NULL){
	// If the terminated flag is down, go to the next element
	if (cur->tdata->terminated == 0){
	    prev = cur;
	    cur = cur->next;
	} else { // terminated flag is up
	    if (cur == head){ // special case for the head of the list.
		head = delete_this_element(cur, prev);
		cur = head;
		prev = cur;
	    } else {
		// Delete the current element and move to the element
		// after the one we have deleted.
		cur = delete_this_element(cur, prev);
	    }
	}
    }

    return head;
        
}

/*
 * Sets the terminate flag on the thread with the specified socket number.
 * If socket_number is negative, flags for all threads will be set.
 * returns 1 if successful, 0 if the socket number does not exist or if
 * the socket_number was -1.
 */
int set_terminate_request(threadlist *head, int socket_number)
{
    int set_all = socket_number < 0 ? 1 : 0;
    
    threadlist *cur = head;
    
    while (cur != NULL){
	if (set_all)
	    cur->tdata->termreq = 1;
	else if (cur->tdata->socket == socket_number){
	    cur->tdata->termreq = 1;
	    return 1;
	}
	cur = cur->next;
    }
    return 0;
}


/* Print the given list to stdout. */
void print_list(threadlist *head)
{
    threadlist *node = head;
    
    while (node != NULL){
	printf("Socket: %d, Termreq: %d, Terminated: %d\n", node->tdata->socket, node->tdata->termreq, node->tdata->terminated);
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
	free(prev->tdata->thread_ref);
	free(prev->tdata);
	free(prev);
    }
}

/* Returns the length of the list.  */
int length(threadlist* head)
{
    int len = 0;

    for (; head != NULL; ++len, head = head->next);
    
    return len;
}
