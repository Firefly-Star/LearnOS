#ifndef USPINLOCK_H
#define USPINLOCK_H

struct uspinlock
{
    volatile int locked;
    char* name;
};

void    uinitlock(struct uspinlock* lk, char* name);
void    uacquire(struct uspinlock *lk);
void    urelease(struct uspinlock *lk);


#endif