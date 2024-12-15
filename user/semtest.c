#include "user/user.h"
#include "kernel/IPC.h"
#include "kernel/semaphore.h"

int main()
{
    int semid = semget(100, 1, IPC_CREATE | 0x666);
    struct sembuf op;
    op.sem_index = 0;
    op.sem_op = 1;
    semop(semid, &op, 1);
    printf("%d\n", semid);
    if(fork())
    {
        struct sembuf op;
        op.sem_index = 0;
        op.sem_op = -1;
        semop(semid, &op, 1);
        printf("parent proc.\n");
        op.sem_op = 1;
        semop(semid, &op, 1);
        int x;
        wait(&x);
    }
    else
    {
        struct sembuf op;
        op.sem_index = 0;
        op.sem_op = -1;
        semop(semid, &op, 1);
        printf("child proc.\n");
        op.sem_op = 1;
        semop(semid, &op, 1);
    }
    exit(0);
}