#include "semaphore.h"
#include "defs.h"
#include "slab.h"

struct spinlock g_sem_lock;
struct kmem_cache g_proclist_slab;

void sem_init()
{
    initlock(&g_sem_lock, "g_sem_lock");
    kmem_cache_create(&g_proclist_slab, "g_proclist_slab", sizeof(struct proclist), 8, 1);
}