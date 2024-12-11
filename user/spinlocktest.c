#include "user/user.h"
#include "user/uspinlock.h"

int main()
{
    struct uspinlock lk;
    uinitlock(&lk, "uspinlock");
    if (fork())
    {
        uacquire(&lk);
        printf("parent proc.\n");
        urelease(&lk);
    }
    else
    {
        uacquire(&lk);
        printf("child proc.\n");
        urelease(&lk);
    }
    int childpid;
    wait(&childpid);
    exit(0);
}