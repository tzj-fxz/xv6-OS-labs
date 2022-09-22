#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int fd1[2];
    int fd2[2];
    int ret1 = pipe(fd1);
    int ret2 = pipe(fd2);
    if (ret1 != 0 || ret2 != 0)
    {
        printf("create pipe error\n");
        exit(1);
    }
    char ch[5] = {0};
    uint64 id = fork();
    if (id == 0)
    {
        // child
        close(fd1[1]);
        close(fd2[0]);
        if (read(fd1[0], ch, 1) != 0)
            printf("%d: received ping\n", getpid());
        write(fd2[1], ch, 1);
        exit(0);
    }
    else
    {
        // parent
        close(fd1[0]);
        close(fd2[1]);
        write(fd1[1], "a", 1);
        if (read(fd2[0], ch, 1) != 0)
            printf("%d: received pong\n", getpid());
        exit(0);
    }
    exit(0);
}