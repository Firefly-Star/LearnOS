#ifndef IPC_H
#define IPC_H

// 命令
#define IPC_SET         0
#define IPC_RMID        1
#define IPC_STAT        2

// 属性
#define IPC_EXCL        1U << 14
#define IPC_CREATE      1U << 15
#define IPC_NOWAIT      1U << 16

// 权限控制
#define IPC_OWNER_MASK  15U << 8
#define IPC_GROUP_MASK  15U << 4
#define IPC_OTHER_MASK  15U
#define IPC_R           1 << 1
#define IPC_W           1 << 2
#define IPC_X           1 << 3

// 可见性控制
#define IPC_PRIVATE     0 // 表示匿名的ipc，用于进程内的标识

enum ipcstate
{
    IPC_UNUSED, IPC_ZOMBIE, IPC_USED
};

#endif