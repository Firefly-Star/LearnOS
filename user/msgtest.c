#include "user.h"

#if 0
int main()
{
    volatile int msqid = msgget(100, 0, IPC_CREATE); // 创建一个键值为100，限长为16（操作系统默认限长）的消息队列
    printf("%d.\n", msqid);
    if (fork())
    { // 父进程作为接收进程
        for (int i = 1;i <= 10; ++i)
        {
            struct msgbuf buf;
            // 仅接受消息类型为i的信息，且不接受消息中长度超过2 * i的部分，阻塞式接收
            msgrcv(msqid, &buf, 2 * i, i, 0);
            printf("mtype: %d, mtext: %s\n", buf.mtype, buf.mtext);
        }
    }
    else
    { // 子进程作为发送进程
        for (int i = 10;i >= 1; --i)
        { // 为了测试，让子进程逆序发送
            struct msgbuf buf;
            buf.mtype = i; // 消息类型为i
            buf.length = 13;
            strcpy(buf.mtext, "Hello parent.");
            msgsnd(msqid, &buf, 0); // 阻塞式发送
        }
    }
    wait(NULL);
    msgctl(msqid, IPC_RMID, NULL);
    exit(0);
}
#endif

#if 1

int main()
{
    int msqid = msgget(100, 0, IPC_CREATE); // 创建一个键值为100，限长为16（操作系统默认限长）的消息队列
    mutex_id mutex = init_mutex(42);
    if (fork())
    { // 父进程作为消费者进程
        struct msgbuf startmsg;
        startmsg.length = 0;
        startmsg.mtype = 1; // 类型为1的消息指示生产者可以进行生产
        msgsnd(msqid, &startmsg, 0); // 启动生产
        for (int i = 0;i < 10; ++i)
        {
            struct msgbuf buf;
            msgrcv(msqid, &buf, 0, 2, 0); // 收到类型为2的消息后开始消费
            acquire_mutex(mutex);
            printf("consumer proc.\n");
            release_mutex(mutex);
            buf.mtype = 1;
            msgsnd(msqid, &buf, 0); // 让生产者继续生产
        }
    }
    else
    { // 子进程作为生产者进程
        for (int i = 0;i < 10; ++i)
        {
            struct msgbuf buf;
            msgrcv(msqid, &buf, 0, 1, 0); // 收到类型为1的消息后开始生产
            acquire_mutex(mutex);
            printf("producer proc.\n");
            release_mutex(mutex);
            buf.mtype = 2;
            msgsnd(msqid, &buf, 0); // 让消费者继续消费
        }
    }
    msgctl(msqid, IPC_RMID, NULL);
    exit(0);
}

#endif