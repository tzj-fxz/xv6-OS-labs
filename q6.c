#include<stdio.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
int main()
{
    int p = fork();
    if (p == 0){
        printf("child\n");
    }
    else {
        int status;
        pid_t t = waitpid(p, &status, WUNTRACED);
        printf("%d\n", t);
    }
    return 0;
}

// 当需要等待子进程组/全部子进程时，或者获取未终止/暂停的子进程的消息，waitpid会有用
