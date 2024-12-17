#include "user.h"
#include "kernel/IPC.h"

int main()
{
    int x1 = shmget(1, 1000, IPC_CREATE | 0x0666);
    int x2 = shmget(1, 1000, 0);
    int x3 = shmget(4097, 200, IPC_CREATE | IPC_EXCL);
    void* ptr = shmat(x2, 0, 0);
    int* x = (int*)(malloc(sizeof(int)));
    *x = -100;
    int y = -100;
    printf("before vfork: %d.\n", *x);
    if (vfork())
    {
        printf("%d, %d, %d\n", x1, x2, x3);
        *((unsigned int*)(ptr)) = 100;
        printf("%lx\n", (uint64)ptr);
        printf("parent proc shm content: %lu.\n", *((uint64*)(ptr)));
        shmctl(x2, IPC_RMID, 0);
        printf("parent vfork: %d.\n", *x);
        printf("parent vfork: %d.\n", y);
    }
    else
    {
        //int x1 = shmget(1, 0, 0);
        //void* ptr = shmat(x1, 0, 0);
        printf("child proc shm content: %lu.\n", *((uint64*)(ptr)));

        //int x2 = shmget(4097, 0, 0);
        struct shmid_ds bf;
        shmctl(x2, IPC_STAT, &bf);
        printf("x2 state: %d, %d, %d, %d.\n", bf.flag, bf.ref_count, bf.state, bf.sz);
        shmctl(x3, IPC_RMID, 0);
        *x = 100;
        printf("child vfork: %d.\n", *x);
        y = 100;
    }
    int cstate;
    x3 = x1;
    wait(&cstate);
    exit(0);
}