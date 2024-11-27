#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "spinlock.h"
#include "proc.h"

struct semaphore
{
    int resource;
    struct spinlock lk;
    struct procQueueNode* queue;

    // for debug
    char* name;
};

#endif