#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "spinlock.h"
#include "proc.h"

struct sem_t{
    struct spinlock lock;   // 24B
    struct proc* first;     // 8B
    struct proc* last;      // 8B
    uint32 value;           // 4B
};

struct sem_file{
    struct sem_t s;
    int reference;
};

#endif