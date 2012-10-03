#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

//strfunc.c
int strcomp(char*, char*);

//lsdir.c

//strlist.c
struct strlist{
    char *s;
    struct strlist *next;
    struct strlist *prev;
};
typedef struct strlist strlist;

strlist* init_list(char*);
int length(strlist*);
int insert_ordered(strlist*, char*);
strlist* insert(strlist*, char*);
void print_list(strlist*);


