#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "spinlock.h"
#include "proc.h"
#include "ipc_hash.h"
#include "IPC.h"

#define SEMMAX 4096
typedef int ipc_id;

struct wait_queue_node
{
    struct proc* p;
    struct wait_queue_node* next;
};

struct wait_queue
{
    struct wait_queue_node* first;
    struct wait_queue_node* last;
};

struct sem
{
    int         value;
    struct wait_queue queue;
};

struct sembuf
{
    uint        sem_index;
    uint16      sem_op;
    uint16      sem_flg;
};

struct semblock {
    struct spinlock lk;         
    ipc_id          id;             // IPC 资源 ID
    uint            sz;             // 信号量数量
    int             ref_count;      // 当前附加的进程数
    uint            flag;           // 权限
    key_t           key;            // 键值
    struct sem*     ptr;            // 信号量集
    enum ipcstate   state;          // 该信号量集的状态
};

struct semid_ds // 用户用以修改和获取semid信息的结构体
{
    uint            sz;             
    int             ref_count;      
    uint            flag;           
    enum ipcstate   state;          
};

struct sem_t
{

};

#endif