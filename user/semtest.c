#include "user/user.h"
#include "kernel/IPC.h"
#include "kernel/semaphore.h"

int main()
{
    int semid = semget(100, 1, IPC_CREATE | 0x666);
    union semun arg;
    arg.val = 1;
    semctl(semid, 0, SETVAL, arg);
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
        semctl(semid, 0, IPC_RMID, arg);
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