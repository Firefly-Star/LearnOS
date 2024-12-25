#include "user/user.h"
#include "kernel/IPC.h"
#include "kernel/semaphore.h"

#if 0
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

#endif

#if 1

int main()
{
    mutex_id mutex = init_mutex(42);
    if (fork())
    {
        acquire_mutex(mutex); // 用互斥锁进行临界区的保护
        printf("parent output.\n");
        release_mutex(mutex);
    }
    else
    {
        acquire_mutex(mutex); // 用互斥锁进行临界区的保护
        printf("child output.\n");
        release_mutex(mutex);
    }
    wait(NULL);
    destruct_mutex(mutex); 
    exit(0);
}

#endif

#if 1

int main()
{
    mutex_id mutex = init_mutex(2);
    ipc_id semophore = semget(42, 2, IPC_CREATE | IPC_EXCL | 0x0666);
    int16 val[2] = {0, 1}; // 信号量集内有2个信号量，分别指示缓冲区的full和empty
    union semun arg; arg.array = val;
    semctl(semophore, 0, SETALL, arg);
    if (fork())
    { // 消费者进程
        for (int i = 0;i < 10; ++i)
        {
            struct sembuf op;
            op.sem_flg = 0; 
            op.sem_index = 0;
            op.sem_op = -1; 
            semop(semophore, &op, 1); // 消耗一个full
            
            acquire_mutex(mutex); // 保护临界区
            printf("consumer proc.\n");
            release_mutex(mutex);
            
            op.sem_index = 1;
            op.sem_op = 1;
            semop(semophore, &op, 1); // 产出一个empty
        }
    }
    else
    { // 生产者进程
        for (int i = 0;i < 10; ++i)
        {
            struct sembuf op;
            op.sem_flg = 0; 
            op.sem_index = 1;
            op.sem_op = -1; 
            semop(semophore, &op, 1); // 消耗一个empty
            
            acquire_mutex(mutex); // 保护临界区
            printf("productor proc.\n");
            release_mutex(mutex);
            
            op.sem_index = 0;
            op.sem_op = 1;
            semop(semophore, &op, 1); // 产出一个full
        }
    }

    wait(NULL);
    semctl(semophore, 0, IPC_RMID, arg); // 将信号量置为IPC_ZOMBIE
    destruct_mutex(mutex); // 析构互斥锁
    exit(0);
}

#endif