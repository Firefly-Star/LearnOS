#ifndef SHM_H
#define SHM_H

#include "types.h"
typedef unsigned int ipc_id;

struct shmid_kernel {
    ipc_id          id;             // IPC 资源 ID
    uint            sz;             // 共享内存段大小
    int             ref_count;      // 当前附加的进程数
    
};

#endif