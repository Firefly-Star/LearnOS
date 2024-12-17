#include "semaphore.h"
#include "defs.h"
#include "slab.h"
#include "proc.h"

struct semblock semid_pool[SEMMAX];

HASH_FUNC(SEMMAX);

AT_FUNC(sem, SEMMAX)

static void insert_last(struct wait_queue* queue, struct proc* p)
{
    struct wait_queue_node* newnode = (struct wait_queue_node*)(kmalloc(sizeof(struct wait_queue_node)));
    newnode->p = p;
    newnode->next = NULL;
    if (queue->first == NULL)
    {
        queue->first = newnode;
        queue->last = newnode;
    }
    else
    {
        queue->last->next = newnode;
        queue->last = newnode;
    }
}

static struct proc* get_first(struct wait_queue* queue)
{
    if (queue->first == NULL)
        return NULL;

    return queue->first->p;
}

static void erase_first(struct wait_queue* queue)
{
    if (queue->first == NULL)
        panic("sem: erase_first.");

    struct wait_queue_node* fnode = queue->first;
    queue->first = fnode->next;
    if (fnode->next == NULL)
    {
        queue->last = NULL;
    }
    kmfree(fnode, sizeof(struct wait_queue_node));
}

void sem_reinit(struct semblock* b)
{
    if (b->ref_count != 0)
        panic("sem_reinit");

    kmfree(b->ptr, sizeof(struct sem) * b->sz);
    b->sz = 0;
    b->flag = 0;
    b->key = 0;
    b->ptr = NULL;
    b->state = IPC_UNUSED;
}

void seminit()
{
    for (int i = 0;i < SEMMAX; ++i)
    {
        initlock(&semid_pool[i].lk, "semblock");
        semid_pool[i].id = i;
        semid_pool[i].key = 0;
        semid_pool[i].ptr = NULL;
        semid_pool[i].ref_count = 0;
        semid_pool[i].sz = 0;
        semid_pool[i].state = IPC_UNUSED;
    }
}

// 需要持有b->lk
void proc_addsem(struct proc* p, struct semblock* b, uint flag)
{
    struct proc_semblock* newnode = (struct proc_semblock*)kmalloc(sizeof(struct proc_semblock));
    newnode->flag = flag;
    newnode->next = p->proc_semhead.next;
    newnode->sem = b;
    p->proc_semhead.next = newnode;
}

// 需要持有b->lk
int proc_remsem(struct proc* p, struct semblock* b)
{
    struct proc_semblock* prev = &(p->proc_semhead);
    while(prev->next != NULL && prev->next->sem != b)
    {
        prev = prev->next;
    }
    if (prev->next == NULL)
    {
        return -1;
    }
    struct proc_semblock* toremove = prev->next;
    ASSERT(toremove->sem == b, "proc_remsem");
    prev->next = toremove->next;
    toremove->sem->ref_count -= 1;
    if (toremove->sem->state == IPC_ZOMBIE && toremove->sem->ref_count == 0)
    {
        sem_reinit(b);
    }
    kmfree(toremove, sizeof(struct proc_semblock));
    return 0;
}

void init_procsemblock(struct proc_semblock* b)
{
    b->flag = 0;
    b->next = NULL;
    b->sem = NULL;
}

ipc_id semget(key_t key, uint64 size, uint flag)
{
    ipc_id id = sem_at(key);
    struct proc* p = myproc();
    if (flag & IPC_CREATE)
    {
        acquire(&semid_pool[id].lk);
        if ((flag & IPC_EXCL) && (semid_pool[id].state != IPC_UNUSED))
        {
            release(&semid_pool[id].lk);
            return -2; // 已有
        }
        if (semid_pool[id].state == IPC_UNUSED)
        { // 创建一个size大小的sem数组
            struct sem* sem = (struct sem*)kmalloc(size * sizeof(struct sem));
            memset(sem, 0, size * sizeof(struct sem));

            semid_pool[id].flag = flag & (IPC_OWNER_MASK | IPC_OTHER_MASK | IPC_GROUP_MASK);
            semid_pool[id].key = key;
            semid_pool[id].ptr = sem;
            semid_pool[id].sz = size;
            semid_pool[id].state = IPC_USED;
            proc_addsem(p, semid_pool + id, semid_pool[id].flag);
            semid_pool[id].ref_count += 1;
            release(&semid_pool[id].lk);
            return id;
        }
        else
        { // 返回已有的sem块号
            proc_addsem(p, semid_pool + id, semid_pool[id].flag);
            semid_pool[id].ref_count += 1;
            release(&semid_pool[id].lk);
            return id;
        }
    }
    else
    {
        acquire(&semid_pool[id].lk);
        if (semid_pool[id].state == IPC_UNUSED)
        { // 不存在该键值的sem
            release(&semid_pool[id].lk);
            return -1;
        }
        else
        { // 返回已有的sem块号
            proc_addsem(p, semid_pool + id, semid_pool[id].flag);
            semid_pool[id].ref_count += 1;
            release(&semid_pool[id].lk);
            return id;
        }
    }
}

int semop(int semid, struct sembuf* sops, uint nsops)
{
    if (semid >= SEMMAX)
    {
        return -1; // 越界
    }

    acquire(&semid_pool[semid].lk);
    if (semid_pool[semid].state == IPC_UNUSED)
    {
        release(&semid_pool[semid].lk);
        return -2; // 无效的semid
    }
    release(&semid_pool[semid].lk);

    struct proc* p = myproc();
    struct sembuf* buf = (struct sembuf*)(kmalloc(nsops * sizeof(struct sembuf)));
    copyin(p->pagetable, (char*)(buf), (uint64)(sops), sizeof(struct sembuf) * nsops);
    
    acquire(&semid_pool[semid].lk);
    for (int i = 0;i < nsops; ++i)
    {
        if (semid_pool[semid].sz <= buf[i].sem_index)
        {
            release(&semid_pool[semid].lk);
            return -3; // 未知信号量索引
        }

        struct sem* sem = semid_pool[semid].ptr + buf[i].sem_index;
        if (buf[i].sem_op < 0)
        { // wait
            if (sem->value < (-buf[i].sem_op))
            { // 资源不够用
                insert_last(&sem->queue, p);
                p->sem_want = (-buf[i].sem_op);
                sleep(p, &semid_pool[semid].lk);
                // 被唤醒后从这里继续运行
            }
            else
            { // 资源够用
                sem->value += buf[i].sem_op;
            }
        }
        else if(buf[i].sem_op > 0)
        { // signal
            sem->value += buf[i].sem_op;
            int stop_wakeup = 0;
            while(!stop_wakeup)
            {
                struct proc* firstsleep = get_first(&sem->queue);
                if (firstsleep == NULL || firstsleep->sem_want > sem->value)
                { // 等待队列为空或资源不够用
                    stop_wakeup = 1;
                }
                else
                {
                    wakeup(firstsleep);
                    sem->value -= firstsleep->sem_want;
                    firstsleep->sem_want = 0;
                    erase_first(&sem->queue);
                }
            }
        }
        // sem_op == 0就是啥也不干找茬来的
    }
    release(&semid_pool[semid].lk);
    return 0;
}

// 依旧没有做权限管理。主要是没有区分用户
int semctl(int semid, int semnum, int cmd, uint64 arg) // 我可以信任你吗? union?
{
    if (semid >= SEMMAX)
    {
        return -1; // 越界
    }

    acquire(&semid_pool[semid].lk);
    if (semid_pool[semid].state == IPC_UNUSED)
    {
        release(&semid_pool[semid].lk);
        return -2; // 无效的semid
    }

    union semun un;
    struct proc* p = myproc();
    switch(cmd)
    {
        case IPC_RMID:
        {
            semid_pool[semid].state = IPC_ZOMBIE;
            proc_remsem(p, semid_pool + semid);
            // TODO: 当引用计数为0时销毁
            break;
        }
        case IPC_STAT:
        {
            un.buf = (struct semid_ds*)(arg);
            struct semid_ds ds;
            ds.flag = semid_pool[semid].flag;
            ds.ref_count = semid_pool[semid].ref_count;
            ds.state = semid_pool[semid].state;
            ds.sz = semid_pool[semid].sz;
            copyout(p->pagetable, (uint64)(un.buf), (char*)(&ds), sizeof(struct semid_ds));
            break;
        }
        case GETALL:
        {
            un.array = (int16*)(arg);
            int16* ar = (int16*)(kmalloc(sizeof(int16) * semid_pool[semid].sz));
            for (int i = 0;i < semid_pool[semid].sz; ++i)
            {
                ar[i] = semid_pool[semid].ptr[i].value;
            }
            copyout(p->pagetable, (uint64)(un.array), (char*)(ar), sizeof(int16) * semid_pool[semid].sz);
            kmfree(ar, sizeof(int16) * semid_pool[semid].sz);
            break;
        }
        case GETVAL:
        {
            int16 v = semid_pool[semid].ptr[semnum].value;
            return v;
        }
        case SETALL:
        {
            un.array = (int16*)(arg);
            int16* ar = (int16*)(kmalloc(sizeof(int16) * semid_pool[semid].sz));
            copyin(p->pagetable, (char*)(ar), (uint64)(un.array), sizeof(int16) * semid_pool[semid].sz);
            for (int i = 0;i < semid_pool[semid].sz; ++i)
            {
                semid_pool[semid].ptr[i].value = ar[i];
            }
            kmfree(ar, sizeof(int16) * semid_pool[semid].sz);
            break;
        }
        case SETVAL:
        {
            un.val = (int16)(arg);
            semid_pool[semid].ptr[semnum].value = un.val;
            break;
        }
        default:
        {
            return -1; // 未知的cmd
        }
    }
    release(&semid_pool[semid].lk);

    return 0;
}