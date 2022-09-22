#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    if (argc != 2){
        printf("Must be only one argument!");
        exit(1);
    }
    int mytick = atoi(argv[1]);
    if (sleep(mytick) != 0){
        printf("Error");
        exit(1);
    }
    exit(0);
}