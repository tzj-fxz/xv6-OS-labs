#include<stdio.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
int main()
{
    int fd[2];
    pipe(fd);
    int p = fork();
    if (p == 0){
        // first child
        write(fd[1], "hello from first child", 23);
    }
    else {
        int p2 = fork();
        if (p2 == 0){
            // second child
            char buf[32];
            read(fd[0], buf, 23);
            printf("%s\n", buf);
        }
        else{
            int status;
            wait(&status);
        }
    }
    return 0;
}


