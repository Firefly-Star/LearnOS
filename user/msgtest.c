#include "user.h"

int main()
{
    int msqid = msgget(100, 0, IPC_CREATE);
    printf("%d.\n", msqid);
    if (fork())
    {
        for (int i = 1;i <= 10; ++i)
        {
            struct msqid_ds ds;
            msgctl(msqid, IPC_STAT, &ds);
            printf("msg_count: %d, maxlen: %d, ref_count: %d, state: %d, flag: %d.\n", ds.msg_count, ds.maxlen, ds.ref_count, ds.state, ds.flag);
            struct msgbuf buf;
            msgrcv(msqid, &buf, i, i, 0);
            printf("mtype: %d, mtext: %s\n", buf.mtype, buf.mtext);
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
    msgctl(msqid, IPC_RMID, NULL);
    exit(0);
}