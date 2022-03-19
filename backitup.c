#include<stdlib.h>
#include<unistd.h>
#include<stdio.h>
#include<dirent.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>
#include<errno.h>   


void backup(char *wd, char *td, int mode);
char *concat(char *st1, char *str2);  // Auto memory allocate to join the two strings

void main() {
    
    int restore = 0;
    char *wd = getcwd(NULL, 0); 
    char *target_dir =  concat(wd, "/.backup");
    printf("Concatted string: %s\n", target_dir);

    backup(wd, target_dir, restore);
    free(target_dir);
}



char *concat(char *str1, char* str2) {
    // strlen exludes the null byte so len + 1 is target allocation
    size_t len = strlen(str1) + strlen(str2);  
    char *new_str = malloc(len + 1); 
    strcat(new_str, str1);
    strcat(new_str, str2);
    return new_str;
    printf("The string is length: %lu\n", len);
}


void backup(char *wd, char* td, int mode) {
    
}
