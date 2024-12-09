#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "spinlock.h"
#include "proc.h"

struct proclist{
    struct proc* current;
    struct proclist* next;
};

struct sem_t{
    struct spinlock lock;
    struct proclist* first;
    struct proclist* last;
    uint32 value;
};

struct sem_file{
    struct sem_t s;
    int reference;
};

#endif