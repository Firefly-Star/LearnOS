#include "shm.h"
#include "defs.h"
#include "ipc.h"

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
