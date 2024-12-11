#include "user.h"
#include "kernel/ipc.h"

int main()
{
    int x1 = shmget(1, 1000, 0);
    int x2 = shmget(1, 1000, IPC_CREATE);
    int x3 = shmget(1, 1000, IPC_CREATE | IPC_EXCL);
    int x4 = shmget(1, 1000, 0);
    printf("%d, %d, %d, %d", x1, x2, x3, x4);
    exit(0);
}