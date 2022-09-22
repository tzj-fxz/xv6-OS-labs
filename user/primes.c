#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    if (argc > 1){
        printf("error argument\n");
        exit(1);
    }
    // pipe pipelines
    int fd[30][2];
    int rootfd[2];
    int fdnum = 0;
    if (pipe(fd[0]) != 0){
        printf("create 0-th pipe error\n");
        exit(1);
    }
    if (pipe(rootfd) != 0){
        printf("create pipe error\n");
        exit(1);
    }
    int id = fork();

    if (id == 0){
        // process pipelines
        close(rootfd[0]);
        while (1){
            close(fd[fdnum][1]);
            int pipe_done = 0;
            int p;
            if (read(fd[fdnum][0], &p, sizeof(p)) == 0){
                if (fdnum == 0){
                    close(rootfd[1]);
                }
                exit(0);
            }
            printf("prime %d\n", p);
            while (1){
                int n;
                if (read(fd[fdnum][0], &n, sizeof(n)) == 0){
                    close(fd[fdnum][0]);
                    exit(0);
                }
                if (n % p != 0){
                    // meet another prime
                    if (pipe_done == 0){
                        // to create a new process only once
                        if (pipe(fd[fdnum+1]) != 0){
                            printf("create %d-th pipe error\n", fdnum);
                        }
                        int child_id = fork();
                        if (child_id == 0){
                            // child
                            // change the pipe num
                            ++fdnum;
                            // back to upper loop
                            break;
                        }
                        else{
                            // this process
                            pipe_done = 1;
                            if (write(fd[fdnum+1][1], &n, sizeof(n)) == -1){
                                printf("write error\n");
                            }
                        }
                    }
                    else{
                        // already create "child" process, but also a prime
                        if (write(fd[fdnum+1][1], &n, sizeof(n)) == -1){
                            printf("write error\n");
                        }
                    }
                }
            }
        }
    }
    else{
        // root
        close(rootfd[1]);
        close(fd[fdnum][0]);
        for (int i = 2; i <= 35; ++i){
            if (write(fd[fdnum][1], &i, sizeof(i)) == -1){
                printf("write error\n");
            }
        }
        close(fd[fdnum][1]);
        int check;
        // while (wait(&check) != id) {}
        while (read(rootfd[0], &check, sizeof(check)) != 0) {}
    }
    exit(0);
}