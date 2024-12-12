#include "uvapg.h"
#include "defs.h"
#include "riscv.h"
#include "memlayout.h"

// TODO: 为每个进程维护自己的diy的slab

void init_freeblock(struct freeblock* head)
{
    head->ptr =  ~0UL;
    head->npg = 0;
    struct freeblock* first = (struct freeblock*)(kmalloc(sizeof(struct freeblock)));
    first->ptr = SHMTOP;
    first->next = NULL;
    first->npg = (SHMTOP - HEAPTOP) / PGSIZE;
    head->next = first;
}

void* uallocva(struct freeblock* head, uint64 npg)
{
    struct freeblock* workptr = head;
    void* result;
    while(workptr->next != NULL && workptr->next->npg < npg)
    {
        workptr = workptr->next;
    }
    if (workptr->next->npg == npg)
    {
        result = (void*)(workptr->next->ptr);
        struct freeblock* newnext = workptr->next->next;
        kmfree(workptr->next, sizeof(struct freeblock));
        workptr->next = newnext;
    }
    else
    {
        result = (void*)(workptr->next->ptr);
        workptr->next->npg -= npg;
        workptr->next->ptr -= npg * PGSIZE;
    }
    return result;
}

void ufreeva(struct freeblock* head, void* va, uint64 npg)
{
    struct freeblock* workptr = head;
    while(workptr->ptr > (uint64)(va) && (workptr->next == NULL || workptr->next->ptr < (uint64)(va)))
    {
        workptr = workptr->next;
    }
    if (workptr->next == NULL)
    {
        // 看能不能和上面的合并
        if ((workptr->ptr + workptr->npg * PGSIZE) == (uint64)(va))
        {
            workptr->npg += npg;
        }
        else
        {
            struct freeblock* newnode = (struct freeblock*)(kmalloc(sizeof(struct freeblock)));
            newnode->ptr = (uint64)(va);
            newnode->npg = npg;
            newnode->next = NULL;
            workptr->next = newnode;
        }
    }
    else
    {
        int umerge = ((workptr->ptr + workptr->npg * PGSIZE) == (uint64)(va));
        int dmerge = (((uint64)(va) + npg * PGSIZE) == (workptr->next->ptr));
        if (umerge && dmerge)
        {
            // 合并三块内存
            workptr->npg = npg + workptr->npg + workptr->next->npg;
            struct freeblock* newnext = workptr->next->next;
            kmfree((void*)(workptr->next), sizeof(struct freeblock));
            workptr->next = newnext;
        }
        else
        {
            if (umerge)
            {
                // 和上面那块合并
                workptr->npg += npg;
            }
            else if (dmerge)
            {
                // 和下面那块合并
                workptr->next->ptr = (uint64)(va);
                workptr->next->next += npg;
            }
            else
            {
                // 独立成块
                struct freeblock* newnode = (struct freeblock*)(kmalloc(sizeof(struct freeblock)));
                newnode->ptr = (uint64)(va);
                newnode->npg = npg;
                newnode->next = workptr->next;
                workptr->next = newnode;
            }
        }
    }
}