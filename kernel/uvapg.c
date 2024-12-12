#include "uvapg.h"
#include "defs.h"
#include "riscv.h"
#include "shm.h"
#include "memlayout.h"

// TODO: 为每个进程维护自己的diy的slab

void init_freeva(struct freeva* head)
{
    head->ptr =  ~0UL;
    head->npg = 0;
    struct freeva* first = (struct freeva*)(kmalloc(sizeof(struct freeva)));
    first->ptr = SHMTOP;
    first->next = NULL;
    first->npg = (SHMTOP - HEAPTOP) / PGSIZE;
    head->next = first;
}


void* uallocva(struct freeva* head, uint64 npg)
{
    struct freeva* workptr = head;
    void* result;
    while(workptr->next != NULL && workptr->next->npg < npg)
    {
        workptr = workptr->next;
    }
    if (workptr->next->npg == npg)
    {
        result = (void*)(workptr->next->ptr);
        struct freeva* newnext = workptr->next->next;
        kmfree(workptr->next, sizeof(struct freeva));
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

void ufreeva(struct freeva* head, void* va, uint64 npg)
{
    struct freeva* workptr = head;
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
            struct freeva* newnode = (struct freeva*)(kmalloc(sizeof(struct freeva)));
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
            struct freeva* newnext = workptr->next->next;
            kmfree((void*)(workptr->next), sizeof(struct freeva));
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
                struct freeva* newnode = (struct freeva*)(kmalloc(sizeof(struct freeva)));
                newnode->ptr = (uint64)(va);
                newnode->npg = npg;
                newnode->next = workptr->next;
                workptr->next = newnode;
            }
        }
    }
}

void init_procshmblock(struct proc_shmblock* head)
{
    head->flag = 0;
    head->va = NULL;
    head->next = NULL;
    head->shm = NULL;
}

void insert_procshmblock(struct proc_shmblock* head, struct proc_shmblock* newblock)
{
    newblock->next = head->next;
    head->next = newblock;
}

struct proc_shmblock* findprev_procshmblock(struct proc_shmblock* head, ipc_id shmid,const void* va)
{
    struct proc_shmblock* prev = head;
    if (shmid != NULL)
    {
        while(prev->next != NULL && prev->next->shm->id != shmid)
        {
            prev = prev->next;
        }
        if (prev->next == NULL)
        {
            panic("erase procshmblock");
        }
        return prev;
    }
    else
    {
        while(prev->next != NULL && prev->next->va != va)
        {
            prev = prev->next;
        }
        if (prev->next == NULL)
        {
            panic("erase procshmblock");
        }
        return prev;
    }
}