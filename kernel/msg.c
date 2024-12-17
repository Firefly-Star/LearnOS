#include "msg.h"
#include "defs.h"
#include "proc.h"

struct msg_queue msqid_pool[MSGMAX];

HASH_FUNC(MSGMAX)
AT_FUNC(msq, MSGMAX)

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
        msqid_pool[i].ref_count = 0;
        msqid_pool[i].state = IPC_UNUSED;
    }
}

void msg_msg_destruct(struct msg_msg* msg)
{
    kmfree(msg->content.mtext, msg->content.length);
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

ipc_id msgget(key_t key, uint flag)
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