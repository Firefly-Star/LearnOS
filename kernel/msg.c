#include "msg.h"
#include "defs.h"
#include "proc.h"

struct msg_queue msqid_pool[MSGMAX];

HASH_FUNC(MSGMAX)
AT_FUNC(msq, MSGMAX)

// 发送阻塞队列的位置
#define SND_CHAN(msq) (&(msq))
// 接收阻塞队列的位置
#define RCV_CHAN(msq) ((char*)(&(msq)) + 1)

void msq_init()
{
    for (int i = 0;i < MSGMAX; ++i)
    {
        initlock(&msqid_pool[i].lk, "msqid_pool");
        msqid_pool[i].id = i;
        msqid_pool[i].first = NULL;
        msqid_pool[i].last = NULL;
        msqid_pool[i].flag = 0;
        msqid_pool[i].key = 0;
        msqid_pool[i].msg_count = 0;
        msqid_pool[i].maxlen = MSQLEN_MAX;
        msqid_pool[i].ref_count = 0;
        msqid_pool[i].state = IPC_UNUSED;
    }
}

void msg_msg_destruct(struct msg_msg* msg)
{
    kmfree(msg, sizeof(struct msg_msg));
}

void msq_reinit(struct msg_queue* msq)
{
    ASSERT(msq->ref_count == 0 && msq->state == IPC_ZOMBIE, "msq_reinit");
    while(msq->first != NULL)
    {
        struct msg_msg* todestruct = msq->first;
        msq->first = msq->first->next;
        msg_msg_destruct(todestruct);
    }
    msq->last = NULL;
    msq->flag = 0;
    msq->key = 0;
    msq->msg_count = 0;
    msq->maxlen = MSQLEN_MAX;
    msq->state = IPC_UNUSED;
}

// 需要持有msq->lk
void proc_addmsq(struct proc* p, struct msg_queue* msq, uint flag)
{
    struct proc_msgblock* newnode = (struct proc_msgblock*)(kmalloc(sizeof(struct proc_msgblock)));
    newnode->flag = flag;
    newnode->next = p->proc_msghead.next;
    newnode->msq = msq;
    p->proc_msghead.next = newnode;
}

void init_procmsgblock(struct proc_msgblock* b)
{
    b->flag = 0;
    b->next = NULL;
    b->msq = NULL;
}

// maxlen = 0表示采用系统默认的最长长度
ipc_id msgget(key_t key, uint32 maxlen, uint flag)
{
    ipc_id id = msq_at(key);
    struct proc* p = myproc();
    if (flag & IPC_CREATE)
    {
        acquire(&msqid_pool[id].lk);
        if ((flag & IPC_EXCL) && (msqid_pool[id].state != IPC_UNUSED))
        {
            release(&msqid_pool[id].lk);
            return -2; // 已有
        }
        if (msqid_pool[id].state == IPC_UNUSED)
        { 
            msqid_pool[id].flag = flag & (IPC_OWNER_MASK | IPC_OTHER_MASK | IPC_GROUP_MASK);
            msqid_pool[id].key = key;
            if (maxlen > 0)
            {
                msqid_pool[id].maxlen = maxlen;
            }
            msqid_pool[id].state = IPC_USED;
            proc_addmsq(p, msqid_pool + id, msqid_pool[id].flag);
            msqid_pool[id].ref_count += 1;
            release(&msqid_pool[id].lk);
            return id;
        }
        else
        { // 返回已有的msq块号
            proc_addmsq(p, msqid_pool + id, msqid_pool[id].flag);
            msqid_pool[id].ref_count += 1;
            release(&msqid_pool[id].lk);
            return id;
        }
    }
    else
    {
        acquire(&msqid_pool[id].lk);
        if (msqid_pool[id].state == IPC_UNUSED)
        { // 不存在该键值的sem
            release(&msqid_pool[id].lk);
            return -1;
        }
        else
        { // 返回已有的msq块号
            proc_addmsq(p, msqid_pool + id, msqid_pool[id].flag);
            msqid_pool[id].ref_count += 1;
            release(&msqid_pool[id].lk);
            return id;
        }
    }
}

// msgflag: 0(阻塞发送), IPC_NOWAIT(非阻塞发送)
int msgsnd(ipc_id msqid, const struct msgbuf *msgp, int msgflg)
{
    if (msqid >= MSGMAX)
    {
        return -1; // 越界
    }

    acquire(&msqid_pool[msqid].lk);
    if (msqid_pool[msqid].state == IPC_UNUSED)
    {
        release(&msqid_pool[msqid].lk);
        return -2; // 无效的msqid;
    }
    release(&msqid_pool[msqid].lk);

    struct msg_msg* newmsg = (struct msg_msg*)(kmalloc(sizeof(struct msg_msg)));
    struct proc* p = myproc();
    copyin(p->pagetable, (char*)(&newmsg->content), (uint64)(msgp), sizeof(struct msgbuf));

    if(newmsg->content.mtype <= 0)
    {
        return -3; // 无效的mtype
    }

    acquire(&msqid_pool[msqid].lk);
    if ((msqid_pool[msqid].msg_count) >= msqid_pool[msqid].maxlen)
    { // 消息队列长度过长
        if (msgflg & IPC_NOWAIT)
        { // 非阻塞，直接返回-1
            release(&msqid_pool[msqid].lk);
            kmfree(newmsg, sizeof(struct msg_msg));
            return -1;
        }
        else
        { // 阻塞，进程睡觉
            do
            {
                sleep(SND_CHAN(msqid_pool[msqid]), &msqid_pool[msqid].lk);
            }while((msqid_pool[msqid].msg_count) >= msqid_pool[msqid].maxlen);
        }
    }
    if(msqid_pool[msqid].first == NULL)
    {
        msqid_pool[msqid].first = newmsg;
        msqid_pool[msqid].last = newmsg;
        newmsg->next = NULL;
    }
    else
    {
        msqid_pool[msqid].last->next = newmsg;
        msqid_pool[msqid].last = newmsg;
        newmsg->next = NULL;
    }
    msqid_pool[msqid].msg_count += 1;
    release(&msqid_pool[msqid].lk);
    wakeup(RCV_CHAN(msqid_pool[msqid]));
    return 0;
}

// msgflag: 0(阻塞接收), IPC_NOWAIT(非阻塞接收)
int msgrcv(ipc_id msqid, struct msgbuf* msgp, uint32 msgsz, uint32 msgtype, int msgflg)
{
    if (msqid >= MSGMAX)
    {
        return -1; // 越界
    }

    acquire(&msqid_pool[msqid].lk);
    if (msqid_pool[msqid].state == IPC_UNUSED)
    {
        release(&msqid_pool[msqid].lk);
        return -2; // 无效的msqid;
    }
    struct msg_msg* prev = NULL;
    if (msqid_pool[msqid].first == NULL)
    { // 队列为空
        if(msgflg & IPC_NOWAIT)
        {
            release(&msqid_pool[msqid].lk);
            return -1; // 无响应type的消息
        }
        else
        { // 等到非空为止
            do
            {
                sleep(RCV_CHAN(msqid_pool[msqid]), &msqid_pool[msqid].lk);
            } while (msqid_pool[msqid].first == NULL);
        }
    }

    // 此时队列非空
    for (;;)
    { // 为了找到所需要类型的消息的前驱节点
        int found = 0;
        
        prev = msqid_pool[msqid].first;
        if (msgtype == 0 || prev->content.mtype == msgtype)
        { // 第一个就是需要的消息，跳出循环
            prev = NULL;
            break;
        }
        while(prev->next != NULL)
        {
            if(msgtype == 0 || prev->next->content.mtype == msgtype)
            {
                found = 1;
                break;
            }
            else
            {
                prev = prev->next;
            }
        }
        if (!found)
        { // 没找到
            if(msgflg & IPC_NOWAIT)
            {
                release(&msqid_pool[msqid].lk);
                return -1; // 无相应type的消息
            }
            else
            { // 睡眠，醒过来后进行下一轮检查
                sleep(RCV_CHAN(msqid_pool[msqid]), &msqid_pool[msqid].lk);
            }
        }
        else
        { // 找到了，跳出循环
            break;
        }
    }
    
    struct proc* p = myproc();
    struct msg_msg* wantednode = prev == NULL? msqid_pool[msqid].first : prev->next;

    if(prev == NULL)
    { // 队列的第一个消息是想要的
        if (msqid_pool[msqid].first == msqid_pool->last)
        { // 队列中只有这一个
            msqid_pool[msqid].first = NULL;
            msqid_pool[msqid].last = NULL;
        }
        else
        {
            msqid_pool[msqid].first = msqid_pool[msqid].first->next;
        }
    }
    else
    {
        if (msqid_pool[msqid].last == prev->next)
        { // 是最后一个
            prev->next = prev->next->next;
            msqid_pool[msqid].last = prev;
        }
        else
        {
            prev->next = prev->next->next;
        }
    }
    msqid_pool[msqid].msg_count -= 1;

    release(&msqid_pool[msqid].lk);

    if (msgsz > 0 && msgsz < wantednode->content.length)
    {
        memset(wantednode->content.mtext + msgsz, 0, wantednode->content.length - msgsz);
    }
    copyout(p->pagetable, (uint64)(msgp), (char*)(&(wantednode->content)), MSGSIZE_MAX);    
    msg_msg_destruct(wantednode);
    wakeup(SND_CHAN(msqid_pool[msqid]));
    return 0;
}

int msgctl(ipc_id msqid, int cmd, struct msqid_ds* buf)
{
    if (msqid >= MSGMAX)
    {
        return -1; // 越界
    }

    acquire(&msqid_pool[msqid].lk);
    if (msqid_pool[msqid].state == IPC_UNUSED)
    {
        release(&msqid_pool[msqid].lk);
        return -2; // 无效的msqid;
    }

    return 0;
}