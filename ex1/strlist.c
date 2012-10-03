#include "ex1.h"


int main(int argc, char *argv[])
{
    strlist *list = init_list("sup");
    
    int i;
    
    for (i = 0; i < 9; ++i){
	list = insert(list, "yw");
    }
    
    print_list(list);
    
    return 0;
}

/* initialises the list and returns a pointer to the start of the list */
strlist* init_list(char *str)
{
    strlist *head = malloc(sizeof(strlist));
    head->s = str;
    head->next = NULL;
    head->prev = NULL;
        
    return head;
}

strlist* insert(strlist *head, char *str)
{
    printf("head str %s, head next %p\n", head->s, head->next);
    strlist *new = malloc(sizeof(strlist));
    new->s = str;
    printf("new string val %s\n", new->s);
    new->prev = head;
    printf("new prev val %p\n", new->prev);
    head->next = new;
    printf("new next val %p\n", new->next);
    
    return (head = new);
}

int insert_ordered(strlist *head, char *str)
{
    strlist *cur = head;
    
    while (strcomp(str, cur->s) != 1){
	cur = head->next;
    }
    
    
    return 0;

}

void print_list(strlist *head)
{
    strlist *node = head;
    
    while (node != NULL){
	printf("%s\n", node->s);
	node = node->prev;
    }
}

int length(strlist* head)
{
    return 0;
}
