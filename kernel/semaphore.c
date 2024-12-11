#include "semaphore.h"
#include "defs.h"
#include "slab.h"

struct spinlock g_sem_lock;

void sem_init()
{
    initlock(&g_sem_lock, "g_sem_lock");
}