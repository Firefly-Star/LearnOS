#include "user.h"

int main()
{
    int msqid = msgget(100, IPC_CREATE);
    printf("%d.\n", msqid);
}