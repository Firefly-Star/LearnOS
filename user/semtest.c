#include "user/user.h"
#include "kernel/IPC.h"
#include "kernel/semaphore.h"

int main(int argc, char* argv[])
{
    USER_ASSERT(argc == 2, "usage: semtest <test_id>");
    int test_id = atoi(argv[1]);
    switch(test_id)
    {
        case 1: {
            mutex_id mutex = init_mutex(42);
            if (fork())
            {
                acquire_mutex(mutex); // 用互斥锁进行临界区的保护
                printf("parent output: abcdefghijklmnopqrstuvwxyz.\n");
                release_mutex(mutex);
            }
            else
            {
                acquire_mutex(mutex); // 用互斥锁进行临界区的保护
                printf("child output: abcdefghijklmnopqrstuvwxyz.\n");
                release_mutex(mutex);
            }
            wait(NULL);
            destruct_mutex(mutex); 
            exit(0);
            break;
        }
        case 2: {
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
        default: {
            printf("unkwown test_id.\n");
            exit(0);
        }
    }
}