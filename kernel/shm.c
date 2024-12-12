#include "shm.h"
#include "defs.h"
#include "proc.h"
#include "ipc.h"
#include "riscv.h"

struct shmblock shmid_pool[SHMMAX];

void shminit()
{
    for (int i = 0;i < SHMMAX; ++i)
    {
        shmid_pool[i].id = i;
        shmid_pool[i].key = ~0UL;
        shmid_pool[i].ptr = NULL;
        shmid_pool[i].ref_count = 0;
        shmid_pool[i].sz = 0;
    }
}

ipc_id hash(key_t key)
{
    return key % SHMMAX;
}

ipc_id shm_at(key_t key)
{
    ipc_id id = hash(key);
    ipc_id origin_id = id;
    while(shmid_pool[id].key != ~0UL)
    {
        if (shmid_pool[id].key == key)
        {
            return id; // 已有key
        }
        else
        {
            id = (id + 1) % SHMMAX;
            if (id == origin_id)
            {
                return -1; // 满了
            }
        }
    }
    return id; // 没找到key，第一个空闲块
}

ipc_id shmget(key_t key, uint64 size, uint flag)
{
    ipc_id id = shm_at(key);
    if (flag & IPC_CREATE)
    {
        if ((flag & IPC_EXCL) && (shmid_pool[id].key != ~0UL))
        {
            return -2; // 已有
        }
        if (shmid_pool[id].key == ~0UL)
        {
            void* pa = kmalloc(size);
            shmid_pool[id].flag = flag & (IPC_OWNER_MASK | IPC_OTHER_MASK | IPC_GROUP_MASK);
            shmid_pool[id].key = key;
            shmid_pool[id].ptr = pa;
            shmid_pool[id].ref_count = 1;
            shmid_pool[id].sz = size;
            return id;
        }
        else
        {
            ++(shmid_pool[id].ref_count);
            return id;
        }
    }
    else
    {
        if (shmid_pool[id].key == ~0UL)
        {
            return -1;
        }
        else
        {
            ++(shmid_pool[id].ref_count);
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
    if (shmid >= SHMMAX || shmid_pool[shmid].key == ~0UL)
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
    void* freeva;
    if (shmaddr == NULL)
    {
        freeva = uallocva(p->freeva_head, npg);
        mappages(p->pagetable, (uint64)(freeva), npg * PGSIZE, shmid_pool[shmid].ptr, PTE_U | perm);
    }

    return freeva;
}