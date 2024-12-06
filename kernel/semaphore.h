#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "spinlock.h"
#include "proc.h"

struct semaphore
{
    int resource;
    struct spinlock lk;

    // for debug
    char* name;
};

struct proc_list {
    struct proc* p;
    struct proc** next;
};


#endif