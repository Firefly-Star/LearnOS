#include "semaphore.h"
#include "defs.h"

void initSemaphore(struct semaphore* semaphore, int initResource, char* name)
{
    initlock(&semaphore->lk, "semaphore lock");
    semaphore->resource = initResource;
    semaphore->name = name;
}
void acquireSemaphore(struct semaphore* semaphore, int acquireResource)
{
    acquire(&semaphore->lk);
    if (semaphore->resource >= acquireResource)
    {
        semaphore->resource -= acquireResource;
        release(&semaphore->lk);
    }
    else
    {
        
    }

}
void releaseSemaphore(struct semaphore*, int releaseResource)
{

}