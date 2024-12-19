#ifndef SLAB_H
#define SLAB_H

#include "spinlock.h"
#include "types.h"

struct freeblock{
    struct freeblock* next;
};

struct snode{
    struct snode* next;
};

struct lnode{
    char* curpage;
    struct lnode* next;
};

union pagelink{
    struct snode* shead;
    struct lnode* lhead;
};


struct kmem_cache{ // TODO: 对大对象的首个可用对象的地址进行独立存储
    char* name;
    uint16 size;
    uint16 align;
    uint16 unitsz;
    uint64 alloc_count;
    union pagelink head;
    struct spinlock lock;
};

#endif