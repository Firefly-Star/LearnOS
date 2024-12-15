#ifndef IPC_HASH_H
#define IPC_HASH_H

#include "types.h"

typedef int ipc_id;
typedef uint64 key_t;

#define HASH_FUNC(poolsize) \
static ipc_id hash(key_t key) \
{ \
    return key % (poolsize); \
}

// 如果找到了key,那么返回key所在的id,如果没找到key,返回第一个空闲的id,如果满了,返回-1
// 0为保留key，不能有shm的键值为0
#define AT_FUNC(name, poolsize) \
ipc_id name##_at(key_t key) \
{ \
    ipc_id id = hash(key); \
    ipc_id origin_id = id; \
    ipc_id first_free = -1; \
    int find_free = 0; \
    for (;;) \
    { \
        acquire(&name##id_pool[id].lk); \
        if (name##id_pool[id].key == key) \
        { \
            release(&name##id_pool[id].lk); \
            break; \
        } \
        if (!find_free && name##id_pool[id].state == IPC_UNUSED) \
        { \
            find_free = 1; \
            first_free = id; \
        } \
        release(&name##id_pool[id].lk); \
        id = (id + 1) % (poolsize); \
        if (id == origin_id) \
            break; \
    } \
    acquire(&name##id_pool[id].lk); \
    if (name##id_pool[id].key != key) \
    { \
        release(&name##id_pool[id].lk); \
        return first_free; \
    } \
    else \
    { \
        release(&name##id_pool[id].lk); \
        return id; \
    } \
}

#endif