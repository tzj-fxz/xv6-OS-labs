#include<stdio.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
int main()
{
    int fd = open("time.txt", O_WRONLY|O_CREAT|O_TRUNC);
    int p = fork();
    if (p == 0){
        printf("child: %d\n", fd);
        write(fd, "child", 5);
    }
    else {
        printf("parent: %d\n", fd);
        write(fd, "parent", 6);
    }
    return 0;
}

// 都可以访问文件描述符，先父进程写再子进程写