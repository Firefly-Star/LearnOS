#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "time.h"
#include "trace.h"
#include "semaphore.h"
#include "slab.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_getyear(void)
{
    int x = 100;
    int y = 2000;
	return x * y;
}

uint64
sys_gettimeofday(void)
{
    uint64 rawPtr;
    argaddr(0, &rawPtr);
    if(rawPtr < 0)
    {
        return -1;
    }
    struct timeVal currentTime = _gettimeofday();

    if (copyout(myproc()->pagetable, rawPtr, (char*)(&currentTime), sizeof(struct timeVal)) < 0) 
    {
    return -1;
    }
    return 0;
}

uint64
sys_trace(void)
{
    uint64 mask;
    argaddr(0, &mask);
    _trace(mask);
    return 0;
}

uint64
sys_find(void)
{
    uint64 mask;
    argaddr(0, &mask);
    _trace(mask);
    return 0;
}

extern struct spinlock g_sem_lock;

uint64
sys_sleep_for_signal(void)
{
    uint64 raw_ptr;
    argaddr(0, &raw_ptr);
    acquire(&g_sem_lock);
    struct proc* p = myproc();
    struct sem_t ksem;
    copyin(p->pagetable, (char*)(&ksem), raw_ptr, sizeof(struct sem_t));
    release(&g_sem_lock);

    acquire(&ksem.lock);
    if (ksem.first == NULL)
    {
        ksem.first = p;
        ksem.last = p;
        p->wait_next = NULL;
    }
    else
    {
        ksem.last->wait_next = p;
        p->wait_next = NULL;
    }
    copyout(p->pagetable, raw_ptr, (char*)(&ksem), sizeof(struct sem_t));
    release(&ksem.lock);

    acquire(&p->lock);
    p->state = BLOCKED;
    sched();
    release(&p->lock);

    return 0;
}

uint64
sys_wake_up_signal(void)
{
    uint64 raw_ptr;
    argaddr(0, &raw_ptr);
    struct proc* p = myproc();
    struct sem_t ksem;
    copyin(p->pagetable, (char*)(&ksem), raw_ptr, sizeof(struct sem_t));

    acquire(&ksem.lock);
    if (ksem.first == NULL)
    {
        panic("syss_wake_up_signal: empty queue.");
    }
    else
    {
        struct proc* p;
        if (ksem.first == ksem.last)
        {
            p = ksem.first;
            ksem.first = NULL;
            ksem.last = NULL;
        }
        else
        {
            p = ksem.first;
            ksem.first = ksem.first->wait_next;
        }
        acquire(&p->lock);
        p->state = RUNNABLE;
        release(&p->lock);
    }
    copyout(p->pagetable, raw_ptr, (char*)(&ksem), sizeof(struct sem_t));
    release(&ksem.lock);

    return 0;
}

uint64
sys_sem_init(void)
{
    uint64 raw_sem;
    uint64 raw_name;
    uint64 raw_value;
    struct proc* p = myproc();
    argaddr(0, &raw_sem);
    argaddr(1, &raw_name);
    argaddr(2, &raw_value);
    struct sem_t ksem;
    initlock(&ksem.lock, (char*)(raw_name));
    ksem.first = NULL;
    ksem.last = NULL;
    ksem.value = raw_value;
    copyout(p->pagetable, raw_sem, (char*)(&ksem), sizeof(struct sem_t));
    return 0;
}