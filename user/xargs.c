#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"

int main(int argc, char *argv[])
{
    if (argc == 1){
        printf("argument number error\n");
        exit(1);
    }
    // save the filename
    char filename[32];
    strcpy(filename, argv[1]);
    // save the command argument
    char *newargv[MAXARG];
    for (int i = 1; i < argc; ++i){
        newargv[i-1] = (char *)malloc(strlen(argv[i]));
        strcpy(newargv[i - 1], argv[i]);
    }
    char buf[512], *p = buf;
    while (read(0, p, 1) != 0){
        if (*p == '\n'){
            *p = 0;
            newargv[argc - 1] = (char *)malloc(strlen(buf));
            strcpy(newargv[argc - 1], buf);
            int id = fork();
            if (id == 0){
                // child
                exec(filename, newargv);
                // hope not reach
                printf("child process error\n");
            }
            else {
                // parent
                int status;
                while (wait(&status) == id) {}
                p = buf;
                free(newargv[argc - 1]);
            }
        }
        ++p;
    }
    for (int i = 0; i < argc; ++i){
        free(newargv[i]);
    }
    exit(0);
}