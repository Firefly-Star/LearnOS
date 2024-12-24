#include "user.h"

int main()
{
    volatile int y = 0;
    int* p = (int*)(malloc(sizeof(int)));
    *p = 0;
    ipc_id sem = semget(44, 1, IPC_CREATE | 0x666);
    if(fork())
    {
        printf("parent data : %d, %d.\n", y, *p); // 预期结果为100, 200
        struct sembuf op;
        int r = semop(sem, &op, 1);
        printf("semop result: %d\n", r); // 这里信号量已被子进程释放，因此为错误代码-2
    }
    else
    {
        // 这是非常危险的一段代码，事实上vfork应当尽可能地不去修改任何的内存空间，
        // 尽量不去对父进程的任何资源进行修改，否则极有可能引起UB
        y = 100; 
        *p = 200;
        union semun pad;
        semctl(sem, 0, IPC_RMID, pad);
    }
    exit(0);
}