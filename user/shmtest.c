#include "user.h"
#include "kernel/IPC.h"

int main()
{
    int x1 = shmget(1, 1000, 0);
    int x2 = shmget(1, 1000, IPC_CREATE | 0x0666);
    int x3 = shmget(1, 1000, IPC_CREATE | IPC_EXCL);
    int x4 = shmget(1, 1000, 0);
    int x5 = shmget(4097, 1000, IPC_CREATE | IPC_EXCL);
    printf("%d, %d, %d, %d, %d\n", x1, x2, x3, x4, x5);
    void* ptr = shmat(x2, 0, 0);
    *((unsigned int*)(ptr)) = 100;
    printf("%lx\n", (uint64)ptr);
    shmctl(x2, IPC_RMID, 0);

    exit(0);
}