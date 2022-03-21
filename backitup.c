// TODO Move string free-ing responsible to the threads
// if its a file, otherwise main thread will if it's a function


#include<semaphore.h>
#include<pthread.h>
#include<stdlib.h>
#include<unistd.h>
#include<stdio.h>
#include<dirent.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>
#include<errno.h>   


#define BACKUP 0
#define RESTORE 1

struct thread_args {
    char *frompath;
    char *topath;
    char *filename;
    unsigned int thread_number;
    int mode;
};

void cdir(DIR *);
void t_init();
void make_thread(char *, char *, char *, int mode);
void backup(char *wd, char *td, int mode);
char *concat(char *st1, char *str2);  // Auto memory allocate to join the two strings
void thread_main(struct thread_args *);
void teardown_threads(pthread_t *[]);



// Thread management variables
pthread_t **threads;
unsigned int thread_counter = 0;
unsigned int thread_max = 100;  // Dynamically updated if more threads end up being required
unsigned long bytes_written = 0;
sem_t bytes_sem;


void main() {
    

    // PARSE ARG
    // CHECK THAT ./.backup directory already exists or create if not
    t_init();

    int restore = 0;
    char *wd = getcwd(NULL, 0); 
    char *target_dir =  concat(wd, ".backup");
    printf("Concatted string: %s\n", target_dir);

    backup(wd, target_dir, restore);

    printf("Returned to main function\n");


    // TODO thread join function
    // TODO Tally files written (thread count) and bytes written
    teardown_threads(threads);
    free(target_dir);
    free(wd);
    free(threads);
}

void teardown_threads(pthread_t *pthreads[]) {
   // Joining created threads
   for(unsigned int i = 0; i < thread_counter; i++) {
        printf("assigning the next thread for joining\n");
        pthread_t *tmp_thread = pthreads[i];

        printf("worked woo");
        printf("Free-ing thread #%u\n", i);
        pthread_join(*tmp_thread, NULL);
        free(tmp_thread);
   }
}

// Initialize thread management variables
void t_init() {
    threads = malloc(thread_max * sizeof(pthread_t *));
    if(sem_init(&bytes_sem, 0, 1) == -1) {
        printf("Error initializing bytes_sem: %s\n", strerror(errno));
        exit(0);
    }
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
    for(int i = 0; i < (len + 2); i++) new_str[i] = '\0'; // Eliminating existing artifacts
    strcat(new_str, str1);
    strcat(new_str, "/");
    strcat(new_str, str2);
    //printf("Created new string: %s\n", new_str);
    return new_str;
}


void backup(char *wd, char* td, int mode) {
    //  Responsible for free-ing memory associated with frompath, topath 
    //  variables when the file is a directory. Otherwise, the spun up thread is reponsible
    
    DIR *dir = opendir(wd);    
    // Preparing variables for loop
    struct dirent *dirent;
    struct stat from_stat;
    struct stat to_stat;
    char *frompath = NULL;
    char *topath = NULL;
    char *filename = NULL;

    while((dirent = readdir(dir)) != NULL) {
        // Need to ignore . and .. files
        if(ignored_file(dirent->d_name)) continue;
        frompath = concat(wd, dirent->d_name);
        int ret = stat(frompath, &from_stat);            
        if(ret) {
            printf("Error stating the frompath %s: %s\n", frompath, strerror(errno));
        }
        topath = concat(td, dirent->d_name);
        // Stat-ed the current file (dirent)
        if(S_ISDIR(from_stat.st_mode)) {
            // Stat-ing the same directory in the target dir
            //printf("to path: %s\n",topath);
            ret = stat(topath, &to_stat);
            if(ret) {
                // Assuming error means that the directory does not already exist
                ret = mkdir(topath, 0777);
                if(ret) {
                    printf("Error creating directory: %s\n", topath);
                    exit(-1);
                }
            }
            backup(frompath, topath, mode);
            free(frompath);
            free(topath);
        }
        else {
            //Non directory file; copying
            filename = concat("", dirent->d_name); // cleaned up by transfer thread
            make_thread(frompath, topath, filename, mode);
        }
    }
    cdir(dir);
}

void make_thread(char *frompath, char *topath, char *filename, int mode) {
    // Allocates memory for a struct thread_args.
    // Threads should clean this up
   
    // First check if we have room in the pthread array 
    if(thread_counter == thread_max) {
        // TODO Need to reallocate memory for threads, update thread variables 
    }
    struct thread_args *args = malloc(sizeof(struct thread_args));
    args->frompath = frompath;
    args->topath = topath;
    args->filename = filename;
    args->thread_number = thread_counter;
    args->mode = mode;
    pthread_t *new_thread = malloc(sizeof(pthread_t));
    if(pthread_create(new_thread, NULL, (void *) (*thread_main), (void *) args)) {
        printf("Error creating a new thread for file: %s\n", frompath);
    }
    else {
        threads[thread_counter] = new_thread;
        thread_counter++;
    } 
}

void thread_main(struct thread_args *args) {
        // Function executed by file transfer threads
        // 1. stat the two files. Overwrite if appropriate
        // 2. free the args->frompath, args->topath, args->filename, and args itself
        // 3. Get sempahore and update total bytes written
        // 4. profit
        
    // Unpacking variables
    int mode = args->mode;
    unsigned int thread_number = args->thread_number;
    char *filename = args->filename;
    struct stat from_stat;
    stat(args->frompath, &from_stat);
    if(mode == BACKUP)  {
        printf("[THREAD %u] Backing up %s\n", thread_number, filename);
    }
    else {
        printf("[THREAD %u] Restoring %s\n", thread_number, filename);
    }
    struct stat to_stat;
    if(stat(args->topath, &to_stat)) {
        // Assuming error means file doesn't exist. Free to transfer
        // transfer()
    }
    else {
        // File exists. Comparing mtime
        if(from_stat.st_mtime > to_stat.st_mtime) {
            // transfer
            if(mode == BACKUP) {
                printf("[THREAD %u] WARNING: Overwriting %s.bak\n", thread_number, filename);
            }
            else {
                printf("[THREAD %u] WARNING: Overwriting %s\n", thread_number, filename);
            }
            // transfer()
        }
        else {
            // No transfer needed 
            if(mode == BACKUP) {
                printf("[THREAD %u] %s does not need backing up\n", thread_number, filename);
            }
            else {
                printf("[THREAD %u] %s is already the most current version\n", thread_number, filename); 
            }
        }
    }

    free(args->frompath);
    free(args->topath);
    free(filename);
    free(args);

    // TODO Tally bytes written?
    
}

void cdir(DIR *dir) {
    closedir(dir);
}
