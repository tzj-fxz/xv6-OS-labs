#include<stdio.h>

void func(int j)
{
    printf("%d\n", j);
    return;
}

int main()
{
    int i;
    for (int i = 0; i < 10; ++i)
        func(i);
    printf("HELLO\n");
    return 0;
}