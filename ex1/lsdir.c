#include "ex1.h"
#include <errno.h>
#include <dirent.h>

int main(int argc, char *argv[])
{
    if (argc != 2){
	printf("usage: lsdir [directory]\n");
	exit(1);
    }
    
    DIR *dir_ptr;
    struct dirent *entry;
    strlist *list = NULL;
    
    if ((dir_ptr = opendir(argv[1])) == NULL){
	perror("Could not read the specified directory");
	exit(1);
    }

    while ((entry = readdir(dir_ptr)) != NULL){
	list = insert_ordered(list, entry->d_name);
    }

    print_list(list);
    free_list(list);
    free(entry);
    closedir(dir_ptr);
    
    return 0;
}
