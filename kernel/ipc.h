#ifndef IPC_H
#define IPC_H

#define IPC_CREATE      1U << 15
#define IPC_EXCL        15U << 14
#define IPC_OWNER_MASK  15U << 8
#define IPC_GROUP_MASK  15U << 4
#define IPC_OTHER_MASK  15U
#define IPC_R           1 << 2
#define IPC_W           1 << 1
#define IPC_X           1

#endif