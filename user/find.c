#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char *getname(char *path)
{
    char *p;

    // Find first character after last slash.
    for(p=path+strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;
    return p;
}

void findprog(char *cur, char *filename)
{
    // printf("%s\n", cur);
    int fd;
    // find current file/dir
    if ((fd = open(cur, 0)) < 0){
        printf("cannot open %s\n", cur);
        return;
    }
    struct stat st;
    // store info
    if (fstat(fd, &st) < 0){
        printf("cannot stat %s\n", cur);
        close(fd);
        return;
    }
    char buf[512], *p;
    struct dirent de;
    switch (st.type){
        case T_FILE:
            // printf("%s, %s\n", getname(cur), filename);
            if (strcmp(getname(cur), filename) == 0){
                printf("%s\n", cur);
            }
            break;
        case T_DIR:
            if (strlen(cur)+1+DIRSIZ+1 > sizeof(buf)){
                printf("path too long\n");
                break;
            }
            strcpy(buf, cur);
            p = buf + strlen(buf);
            *p++ = '/';
            while (read(fd, &de, sizeof(de)) == sizeof(de)){
                // printf("%s\n", de.name);
                if (de.inum == 0){
                    continue;
                }
                if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0){
                    continue;
                }
                memmove(p, de.name, strlen(de.name));
                p[strlen(de.name)] = 0;
                if (stat(buf, &st) < 0){
                    printf("cannot stats %s\n", buf);
                    continue;
                }
                findprog(buf, filename);
            }
            break;
    }
    close(fd);
}

int main(int argc, char *argv[])
{
    if (argc != 3){
        printf("argument number error\n");
        exit(1);
    }
    findprog(argv[1], argv[2]);
    exit(0);
}