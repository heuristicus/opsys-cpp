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
};
typedef struct strlist strlist;

strlist make_list(char* s);
int length(strlist* head);
int insert(strlist* head, char* s);
