#ifndef IPC_H
#define IPC_H

// 命令
#define IPC_SET         0
#define IPC_RMID        1
#define IPC_STAT        2

// 属性
#define IPC_EXCL        1U << 14 // 已经有该键值的IPC时，会报错
#define IPC_CREATE      1U << 15 // 在有需要的前提下进行IPC的创建
#define IPC_NOWAIT      1U << 16 // 非阻塞式命令

// 权限控制
#define IPC_OWNER_MASK  15U << 8 // flag的第8位到第11位为该IPC所有者的权限位
#define IPC_GROUP_MASK  15U << 4 // flag的第4位到第7位为该IPC所有者的权限位
#define IPC_OTHER_MASK  15U      // flag的第0位到第3位为该IPC所有者的权限位
#define IPC_R           1 << 1 // 读权限
#define IPC_W           1 << 2 // 写权限
#define IPC_X           1 << 3 // 执行权限
#define IPC_C           1 << 4 // 修改权限的权限

// 可见性控制
#define IPC_PRIVATE     0 // 表示匿名的ipc，用于进程内的标识

enum ipcstate
{
    IPC_UNUSED, IPC_ZOMBIE, IPC_USED
};

#endif