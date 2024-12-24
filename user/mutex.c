#include "user.h"

typedef ipc_id mutex_id;

mutex_id init_mutex(key_t key)
{
    mutex_id mid = semget(key, 1, IPC_CREATE | IPC_EXCL | 0x0666);
    union semun val;
    val.val = 1;
    semctl(mid, 0, SETVAL, val);
    return mid;
}

int acquire_mutex(mutex_id mutexid)
{
    struct sembuf op;
    op.sem_flg = 0;
    op.sem_index = 0;
    op.sem_op = -1;
    return semop(mutexid, &op, 1);
}

int release_mutex(mutex_id mutexid)
{
    struct sembuf op;
    op.sem_flg = 0;
    op.sem_index = 0;
    op.sem_op = 1;
    return semop(mutexid, &op, 1);
}

void destruct_mutex(mutex_id mutexid)
{
    union semun pad;
    semctl(mutexid, 0, IPC_RMID, pad);
}