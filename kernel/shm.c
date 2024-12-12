#include "shm.h"
#include "defs.h"
#include "proc.h"
#include "IPC.h"
#include "riscv.h"

struct shmblock shmid_pool[SHMMAX];

// 将一个控制块给初始化
void shm_reinit(struct shmblock* b)
{
    if (b->ref_count != 0)
        panic("shm_reinit.");
        
    kmfree(b->ptr, b->sz);
    b->key = 0;
    b->ptr = NULL;
    b->sz = 0;
    b->state = SHM_UNUSED;
}

void shminit()
{
    for (int i = 0;i < SHMMAX; ++i)
    {
        shmid_pool[i].id = i;
        shmid_pool[i].key = 0;
        shmid_pool[i].ptr = NULL;
        shmid_pool[i].ref_count = 0;
        shmid_pool[i].sz = 0;
        shmid_pool[i].state = SHM_UNUSED;
    }
}

ipc_id hash(key_t key)
{
    return key % SHMMAX;
}

// 如果找到了key,那么返回key所在的id,如果没找到key,返回第一个空闲的id,如果满了,返回-1
// 0为保留key，不能有shm的键值为0
ipc_id shm_at(key_t key)
{
    ipc_id id = hash(key);
    ipc_id origin_id = id;
    ipc_id first_free = -1;
    int find_free = 0;
    while(shmid_pool[id].key != key)
    {
        if (!find_free && shmid_pool[id].state == SHM_UNUSED)
        {
            find_free = 1;
            first_free = id;
        }
        id = (id + 1) % SHMMAX;
        if (id == origin_id)
            break;
    }
    if (shmid_pool[id].key != key)
    { // 没找到
        return first_free;
    }
    else
    { // 找到了
        return id; 
    }
}

ipc_id shmget(key_t key, uint64 size, uint flag)
{
    ipc_id id = shm_at(key);
    if (flag & IPC_CREATE)
    {
        if ((flag & IPC_EXCL) && (shmid_pool[id].state != SHM_UNUSED))
        {
            return -2; // 已有
        }
        if (shmid_pool[id].state == SHM_UNUSED)
        { // 创建一块新的共享内存，此时它的引用计数仍然为0
            void* pa = kmalloc(size);
            shmid_pool[id].flag = flag & (IPC_OWNER_MASK | IPC_OTHER_MASK | IPC_GROUP_MASK);
            shmid_pool[id].key = key;
            shmid_pool[id].ptr = pa;
            shmid_pool[id].sz = size;
            shmid_pool[id].state = SHM_USED;
            return id;
        }
        else
        { // 返回已有的共享内存块号
            return id;
        }
    }
    else
    {
        if (shmid_pool[id].state == SHM_UNUSED)
        { // 不存在该键值的共享内存
            return -1;
        }
        else
        { // 返回已有的共享内存块号
            return id;
        }
    }
}

// 目前shmaddr只能为0，即操作系统来自动分配地址
// TODO: shmaddr不为0，即用户指定地址时，检查shmaddr是否有效
// TODO: 拥有者/组用户/其他用户的权限区分。目前没有用户的概念，所有进程都是拥有者。
void* shmat(int shmid, const void* shmaddr, int shmflag)
{
    struct proc* p = myproc();
    if (shmid >= SHMMAX || shmid_pool[shmid].state == SHM_UNUSED)
    {
        p->errno = 1; // 无效的shmid
        return NULL; 
    }
    uint64 npg = (shmid_pool[shmid].sz + PGSIZE - 1) / PGSIZE;
    
    // 权限处理
    int perm, flag;
    perm = 0;
    if (1)
    {
        flag = (shmid_pool[shmid].flag & IPC_OWNER_MASK) >> 8;
    }
    if (shmflag & SHM_RDONLY)
    {
        flag &= (~IPC_W) & (~IPC_X);
    }
    if (flag & IPC_R)
    {
        perm |= PTE_R;
    }
    if (flag & IPC_W)
    {
        perm |= PTE_W;
    }
    if (flag & IPC_X)
    {
        perm |= PTE_X;
    }

    // 内存映射
    void* freeva = NULL;
    if (shmaddr == NULL)
    {
        freeva = uallocva(p->freeva_head, npg);
        mappages(p->pagetable, (uint64)(freeva), npg * PGSIZE, (uint64)(shmid_pool[shmid].ptr), PTE_U | perm);
        // 记录到进程的共享内存块链表中
        struct proc_shmblock* newblock = (struct proc_shmblock*)(kmalloc(sizeof(struct proc_shmblock)));
        newblock->flag = flag;
        newblock->shm = shmid_pool + shmid;
        newblock->va = freeva;
        insert_procshmblock(p->proc_shmhead, newblock);
        shmid_pool[shmid].ref_count += 1; // 增加引用计数
    }

    return freeva;
}

void shmdt(struct proc* p, const void* shmaddr)
{
    struct proc_shmblock* prev = findprev_procshmblock(p->proc_shmhead, NULL, shmaddr);
    struct shmblock* shm = prev->next->shm;
    uvmunmap(p->pagetable, (uint64)(shmaddr), (shm->sz + PGSIZE - 1) / PGSIZE, 0);
    shm->ref_count -= 1;
    if (shm->state == SHM_ZOMBIE && shm->ref_count == 0)
    {
        printf("shm destructed.\n");
        shm_reinit(shm);
    }
    struct proc_shmblock* newnext = prev->next->next;
    kmfree(prev->next, sizeof(struct proc_shmblock));
    prev->next = newnext;
}

int shmctl(int shmid, int cmd, struct shmid_ds *buf)
{
    if (shmid_pool[shmid].state == SHM_UNUSED)
    {
        return -1; // 无效的shmid
    }
    switch(cmd)
    {
        case IPC_SET: {
            shmid_pool[shmid].flag = buf->flag;
            break;
        }
        case IPC_RMID: {
            shmid_pool[shmid].state = SHM_ZOMBIE;
            break;
        }
        case IPC_STAT: {
            buf->sz = shmid_pool[shmid].sz;
            buf->ref_count = shmid_pool[shmid].ref_count;
            buf->flag = shmid_pool[shmid].flag;
            buf->state = shmid_pool[shmid].state;
        }
        default: {
            return -2; // 未知的命令
        }
    }
    return 0;
}