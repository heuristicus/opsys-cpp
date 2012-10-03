#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

//strfunc.c
int strcomp(char*, char*);
int len(char*);

//strlist.c
struct strlist{
    char *s;
    struct strlist *next;
    //struct strlist *prev;
};
typedef struct strlist strlist;

strlist* init_list(char*);
int length(strlist*);
strlist* insert_ordered(strlist*, char*);
strlist* add(strlist*, char*);
void print_list(strlist*);
void free_list(strlist *);


