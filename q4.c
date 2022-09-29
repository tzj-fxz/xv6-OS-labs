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
        char *myargv[2];
        myargv[0] = "/bin/ls";
        myargv[1] = NULL;
        char *myargv2[1];
        myargv2[0] = "";
        myargv2[1] = NULL;
        execvp("ls", myargv2);
    }
    else {
        int status;
        wait(&status);
    }
    return 0;
}

// execl("/bin/ls", "ls", NULL)         路径+一列参数+NULL
// execle("/bin/ls", "ls", NULL, NULL)  路径+一列参数+NULL+environ
// execlp("ls", "", NULL)               文件名+一列参数+NULL，自动搜索可执行文件
// execv("/bin/ls", myargv)             路径+char*[]一列以NULL结尾的参数
// execve("/bin/ls", myargv, NULL)      路径+char*[]一列以NULL结尾的参数+environ
// execvp("ls", myargv2)                文件名+char*[]一列以NULL结尾的参数，自动搜索可执行文件
