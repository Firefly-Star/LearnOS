#include "uspinlock.h"

void uinitlock(struct uspinlock* lk, char* name)
{
    lk->name = name;
    lk->locked = 0;
}

void uacquire(struct uspinlock *lk)
{
    while(__sync_lock_test_and_set(&lk->locked, 1) != 0)
    {}

    __sync_synchronize();
}

void urelease(struct uspinlock *lk)
{
    __sync_lock_release(&lk->locked);
    __sync_synchronize();
}
