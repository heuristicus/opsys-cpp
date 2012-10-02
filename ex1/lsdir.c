#include "ex1.h"
#include <dirent.h>

int main(int argc, char *argv[])
{
    if (argc != 2){
	printf("usage: lsdir [directory]\n");
	exit(1);
    }
    
    DIR *dir_ptr;
    struct dirent *entry;
    
    if ((dir_ptr = opendir(argv[1])) == NULL){
	printf("Directory invalid.\n");
	exit(1);
    }

    while ((entry = readdir(dir_ptr)) != NULL){
	printf("%s\n", entry->d_name);
    }
    
    free(entry);
    closedir(dir_ptr);
    
    return 0;
}
