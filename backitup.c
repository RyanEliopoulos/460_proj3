#include<stdlib.h>
#include<unistd.h>
#include<stdio.h>
#include<dirent.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>
#include<errno.h>   


void cdir(DIR *);
void backup(char *wd, char *td, int mode);
char *concat(char *st1, char *str2);  // Auto memory allocate to join the two strings

void main() {
    

    // PARSE ARG
    // CHECK THAT ./.backup directory already exists or create if not
    
    int restore = 0;
    char *wd = getcwd(NULL, 0); 
    char *target_dir =  concat(wd, ".backup");
    printf("Concatted string: %s\n", target_dir);

    backup(wd, target_dir, restore);

    printf("Returned to main function\n");
    free(target_dir);
    free(wd);
}


int ignored_file(char *filepath) {
    if(!strcmp(".", filepath)) return 1;
    if(!strcmp("..", filepath)) return 1;
    if(!strcmp(".backup", filepath)) return 1;
    return 0;
}


char *concat(char *str1, char* str2) {
    // strlen exludes the null byte so len + 2 is target allocation
    // 1 for the null byte and 1 for the interpolated '/'
    size_t len = strlen(str1) + strlen(str2);  
    char *new_str = malloc(len + 2); 
    strcat(new_str, str1);
    strcat(new_str, "/");
    strcat(new_str, str2);
    return new_str;
}


void backup(char *wd, char* td, int mode) {
    DIR *dir = opendir(wd);    
    //printf("In backup. wd: %s\ntd:%s\n", wd, td);
    // Preparing variables for loop
    struct dirent *dirent;
    struct stat from_stat;
    struct stat to_stat;

    char *frompath = NULL;
    char *topath = NULL;
    while((dirent = readdir(dir)) != NULL) {
        if(frompath !=NULL) {
            free(frompath);
            frompath = NULL;
            printf("Freeing frompath in loop\n");
        }
        if(topath !=NULL) {
            free(topath);
            topath = NULL;
            printf("freeing topath in loop\n");
        }
        // Need to ignore . and .. files
        if(ignored_file(dirent->d_name)) continue;
        frompath = concat(wd, dirent->d_name);
        int ret = stat(frompath, &from_stat);            
        if(ret) {
            //printf("Error stating the frompath %s: %s\n", frompath, strerror(errno));
        }
        // Stat-ed the current file (dirent)
        if(S_ISDIR(from_stat.st_mode)) {
            // Stat-ing the same directory in the target dir
            topath = concat(td, dirent->d_name);
            //printf("to path: %s\n",topath);
            ret = stat(topath, &to_stat);
            if(ret) {
                // Assuming error means that the directory does not already exist
                ret = mkdir(topath, 0777);
                if(ret) {
                    //printf("Error creating directory: %s\n", topath);
                    exit(-1);
                }
            }
            backup(frompath, topath, mode);
        }
        else {
            //printf("Pretending to copy file: %s\n", frompath);
        }
        
    }
    cdir(dir);
    printf("Exited loop\n");
    if(frompath != NULL) free(frompath);
    printf("freed frompath\n");
    if (topath != NULL) free(topath);
    printf("freed topath\n");

}

void cdir(DIR *dir) {
    closedir(dir);
}
