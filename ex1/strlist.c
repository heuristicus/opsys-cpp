#include "ex1.h"


int main(int argc, char *argv[])
{
    strlist *list = init_list("sup");
    
    insert(list, "wahey");
    insert(list, "wahey");
    insert(list, "wahey");
    insert(list, "wahey");

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

void insert(strlist *head, char *str)
{
    strlist *new = malloc(sizeof(strlist));
    new->s = str;
    new->prev = head;
    head->next = new;
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
    
    while (node->next != NULL){
	printf("%s\n", node->s);
	node = node->next;
    }
	
}

int length(strlist* head)
{
    return 0;
}
