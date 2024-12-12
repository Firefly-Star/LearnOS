#ifndef SHM_H
#define SHM_H

#define SHMMAX 4096

#define SHM_RDONLY 1

#include "types.h"
typedef int ipc_id;
typedef uint64 key_t;

enum shmstate
{
    SHM_UNUSED, SHM_ZOMBIE, SHM_USED
};

struct shmblock {
    ipc_id          id;             // IPC 资源 ID
    uint            sz;             // 共享内存段大小
    int             ref_count;      // 当前附加的进程数
    uint            flag;           // 权限
    key_t           key;            // 键值
    void*           ptr;            // 共享内存的物理地址
    enum shmstate   state;          // 该共享内存的状态
};

struct shmid_ds { // 用户用以修改和获取shmid信息的结构体
    uint            sz;             // 共享内存段大小
    int             ref_count;      // 当前附加的进程数
    uint            flag;           // 权限
    enum shmstate   state;          // 该共享内存的状态
};

#endif