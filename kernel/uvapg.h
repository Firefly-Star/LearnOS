// manage user virtual address page
#ifndef UVAPG_H
#define UVAPG_H

#include "types.h"

// 在用户空间的trap页下面，从上往下地维护一个可用的虚拟地址块链表，采用最早适应的算法
struct freeva
{
    uint64 ptr;
    struct freeva* next;
    uint64 npg;
};

#endif