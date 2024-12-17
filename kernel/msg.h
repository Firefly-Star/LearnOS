#ifndef MSG_H
#define MSG_H

#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "IPC.h"
#include "ipc_hash.h"

#define MSGMAX      16      
#define MSGSIZE_MAX PGSIZE

struct msgbuf
{
    int32   mtype;
    uint32  length;
    char*   mtext;
};

struct msg_msg
{
    struct msgbuf   content;
    struct msg_msg* next;
};

struct msg_queue
{
    struct spinlock lk;
    ipc_id          id;
    struct msg_msg* first;
    struct msg_msg* last;
    uint            flag;
    key_t           key;
    int             msg_count;
    int             ref_count;
    enum ipcstate   state;
};

struct msqid_ds
{
    int             msg_count;
    int             ref_count;
    enum ipcstate   state;
    uint            flag;
};

struct proc_msgblock
{
    uint                flag;
    struct msg_queue*   msq;
    struct proc_msgblock* next;
};

#endif