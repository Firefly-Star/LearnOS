#ifndef SHM_H
#define SHM_H

#define SHMMAX 4096

#define SHM_RDONLY 1

#include "types.h"
#include "spinlock.h"
#include "ipc_hash.h"
#include "IPC.h"

struct shmblock {
    struct spinlock lk;         
    ipc_id          id;             // IPC 资源 ID
    uint            sz;             // 共享内存段大小
    int             ref_count;      // 当前附加的进程数
    uint            flag;           // 权限
    key_t           key;            // 键值
    void*           ptr;            // 共享内存的物理地址
    enum ipcstate   state;          // 该共享内存的状态
};

struct shmid_ds { // 用户用以修改和获取shmid信息的结构体
    uint            sz;             // 共享内存段大小
    int             ref_count;      // 当前附加的进程数
    uint            flag;           // 权限
    enum ipcstate   state;          // 该共享内存的状态
};

#endif