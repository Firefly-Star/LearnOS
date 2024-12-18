#include "user.h"

int main()
{
    int msqid = msgget(100, 10000, IPC_CREATE);
    printf("%d.\n", msqid);
    if (fork())
    {
        for (int i = 1;i <= 10; ++i)
        {
            struct msgbuf buf;
            msgrcv(msqid, &buf, i, i, 0);
            printf("mtype: %d, mtext: %s.\n", buf.mtype, buf.mtext);
        }
    }
    else
    {
        for (int i = 10;i >= 1; --i)
        {
            struct msgbuf buf;
            buf.mtype = i;
            buf.length = 13;
            strcpy(buf.mtext, "Hello parent.");
            msgsnd(msqid, &buf, 0);
        }
    }
    exit(0);
}