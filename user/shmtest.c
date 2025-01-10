#include "user.h"
#include "kernel/IPC.h"

int main(int argc, char* argv[])
{
    USER_ASSERT(argc == 2, "usage: shmtest <test_id>");
    int test_id = atoi(argv[1]);
    switch(test_id)
    {
        case 1: {
            int shmid1 = shmget(1, 1000, IPC_CREATE | 0x0666); // 获取一个键值为1的共享内存标识符
            int shmid2 = shmget(1, 1000, 0); // 返回上面已有的共享内存标识符
            int shmid3 = shmget(1, 1000, IPC_CREATE | IPC_EXCL); // 指定了IPC_EXCL，此时会返回错误值-2
            int shmid4 = shmget(4097, 200, IPC_CREATE | IPC_EXCL); // 获取一个新的共享内存标识符
            void* ptr = shmat(shmid2, 0, 0); // 将键值为1的共享内存给映射到共享内存段
            *((uint64*)ptr) = 0; // 赋一个初值
            printf("shmid1: %d, shmid2: %d, shimid3: %d, shmid4: %d.\n", shmid1, shmid2, shmid3, shmid4); // 期望结果应为1，1，-2，2
            if(fork()) // 使用共享内存在父子进程中进行通信，fork会继承父进程对于共享内存段的引用并复制映射
            { // 父进程
                *((uint64*)ptr) = 100; // 修改共享内存
                sleep(10); // 等待子进程读取和修改共享内存段信息
                printf("parent shm value: %lu.\n", *((uint64*)ptr)); // 获取共享内存段信息，期望结果应为200
                *((uint64*)ptr) = 300; // 修改共享内存
                wait(NULL); // 等待子进程结束
            }
            else
            {
                sleep(5);
                printf("child shm value1: %lu.\n", *((uint64*)ptr)); // 获取共享内存段信息，期望结果应为100
                *((uint64*)ptr) = 200; // 修改共享内存
                sleep(10); // 等待父进程读取和修改共享内存段信息
                printf("child shm value2: %lu.\n", *((uint64*)ptr)); // 获取共享内存段信息，期望结果应为300
            }
            // 这条指令会在父子进程中执行两次，但是不会出错，对一个已经是IPC_ZOMBIE的进程再次IPC_RMID，just do nothing.
            shmctl(shmid1, IPC_RMID, NULL); // 将shm置为IPC_ZOMBIE，在进程结束后由操作系统来回收资源
            exit(0);
            break;
        }
        case 2: {
            int shmid1 = shmget(1, 1000, IPC_CREATE | 0x0666); // 获取一个键值为1的共享内存标识符
            int shmid2 = shmget(1, 1000, 0); // 返回上面已有的共享内存标识符
            int shmid3 = shmget(1, 1000, IPC_CREATE | IPC_EXCL); // 指定了IPC_EXCL，此时会返回错误值-2
            int shmid4 = shmget(4097, 200, IPC_CREATE | IPC_EXCL); // 获取一个新的共享内存标识符
            void* ptr = shmat(shmid2, 0, 0); // 将键值为1的共享内存给映射到共享内存段
            *((uint64*)ptr) = 0; // 赋一个初值
            printf("shmid1: %d, shmid2: %d, shimid3: %d, shmid4: %d.\n", shmid1, shmid2, shmid3, shmid4); // 期望结果应为1，1，-2, -2
            if(fork()) // 使用共享内存在父子进程中进行通信，fork会继承父进程对于共享内存段的引用并复制映射
            { // 父进程
                *((uint64*)ptr) = 100; // 修改共享内存
                sleep(10); // 等待子进程读取和修改共享内存段信息
                printf("parent shm value: %lu.\n", *((uint64*)ptr)); // 获取共享内存段信息，期望结果应为200
                *((uint64*)ptr) = 300; // 修改共享内存
                wait(NULL); // 等待子进程结束
            }
            else
            {
                sleep(5);
                printf("child shm value1: %lu.\n", *((uint64*)ptr)); // 获取共享内存段信息，期望结果应为100
                *((uint64*)ptr) = 200; // 修改共享内存
                sleep(10); // 等待父进程读取和修改共享内存段信息
                printf("child shm value2: %lu.\n", *((uint64*)ptr)); // 获取共享内存段信息，期望结果应为300
            }
            // 这条指令会在父子进程中执行两次，但是不会出错，对一个已经是IPC_ZOMBIE的进程再次IPC_RMID，just do nothing.
            shmctl(shmid1, IPC_RMID, NULL); // 将shm置为IPC_ZOMBIE，在进程结束后由操作系统来回收资源
            shmctl(shmid4, IPC_RMID, NULL); // 将第二块共享内存（键值为4097）也置为IPC_ZOMBIE，它也会被释放。
            exit(0);
            break;
        }
        default: {
            printf("unkwown test_id.\n");
            exit(0);
        }
    }
}