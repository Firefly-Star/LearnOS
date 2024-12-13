#include "user.h"
#include "kernel/IPC.h"

int main()
{
    if (fork())
    {
        int x1 = shmget(1, 1000, IPC_CREATE | 0x0666);
        int x2 = shmget(1, 1000, 0);
        int x3 = shmget(4097, 1000, IPC_CREATE | IPC_EXCL);
        printf("%d, %d, %d\n", x1, x2, x3);
        void* ptr = shmat(x2, 0, 0);
        *((unsigned int*)(ptr)) = 100;
        printf("%lx\n", (uint64)ptr);
        printf("parent proc shm content: %lu.\n", *((uint64*)(ptr)));
        shmctl(x2, IPC_RMID, 0);
    }
    else
    {
        int x1 = shmget(1, 0, 0);
        void* ptr = shmat(x1, 0, 0);
        printf("child proc shm content: %lu.\n", *((uint64*)(ptr)));
    }
    int cstate;
    wait(&cstate);
    exit(0);
}