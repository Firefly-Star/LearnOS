#ifndef IPC_H
#define IPC_H

typedef int key_t;

struct ipc_resource{
    key_t key;                      // 哈希表的键, 4B
    void* resource;                 // 指向具体 IPC 资源的指针, 8B
    struct ipc_resource* next;      // 链表的下一个节点, 8B
}; // 20B

#define TABLE_SIZE 1024
struct ipc_hash_table{
    struct ipc_resource* table[TABLE_SIZE];
};// 20KB

#endif