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
#include "shm.h"
#include "IPC.h"
#include "proc.h"

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

  if(n>0){
    myproc()->sz += n;
  }else if (myproc()->sz > (uint64)(-n)) {
    uvmdealloc(myproc()->pagetable, myproc()->sz, myproc()->sz + n);
  }else {
    return -1;
  }
  
  // Lazy allocation 先不进行内存分配
  // if(growproc(n) < 0)
  //   return -1;


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

uint64
sys_set_priority(void)
{
  uint64 pid, pr;
  
  argaddr(0, &pid);
  argaddr(1, &pr);
  return set_priority(pid, pr);
}

uint64
sys_ticks(void)
{
    return getticks();
}

uint64
sys_shmget()
{
    uint64 key;
    uint64 size;
    uint64 flag;
    argaddr(0, &key);
    argaddr(1, &size);
    argaddr(2, &flag);
    return shmget(key, size, (uint)flag);
}

uint64
sys_shmat(void)
{
    uint64 shmid;
    uint64 shmaddr;
    uint64 shmflag;
    argaddr(0, &shmid);
    argaddr(1, &shmaddr);
    argaddr(2, &shmflag);

    return (uint64)(shmat((int)shmid, (void*)(shmaddr), (int)(shmflag)));
}

uint64 
sys_shmdt(void)
{
    struct proc* p = myproc();
    uint64 raw_addr;
    argaddr(0, &raw_addr);
    shmdt(p, (void*)(raw_addr));
    return 0;
}

uint64
sys_shmctl(void)
{
    struct proc* p = myproc();
    uint64 shmid;
    uint64 cmd;
    uint64 raw_buf;
    argaddr(0, &shmid);
    argaddr(1, &cmd);
    argaddr(2, &raw_buf);
    struct shmid_ds mid;
    int result = shmctl((int)(shmid), (int)(cmd), &mid);
    if (cmd == IPC_STAT)
    {
        copyout(p->pagetable, raw_buf, (char*)(&mid), sizeof(struct shmid_ds));
    }
    return result;
}

uint64
sys_vfork(void)
{
    return vfork();
}

uint64
sys_semget(void)
{
    uint64 key;
    uint64 size;
    uint64 flag;
    argaddr(0, &key);
    argaddr(1, &size);
    argaddr(2, &flag);
    return semget(key, size, (uint)(flag));
}

uint64
sys_semop(void)
{
    uint64 semid;
    uint64 sops;
    uint64 nsops;
    argaddr(0, &semid);
    argaddr(1, &sops);
    argaddr(2, &nsops);
    return semop((int)(semid), (struct sembuf*)(sops), (uint)(nsops));
}

uint64
sys_semctl(void)
{
    uint64 semid;
    uint64 semnum;
    uint64 cmd;
    uint64 un;

    argaddr(0, &semid);
    argaddr(1, &semnum);
    argaddr(2, &cmd);
    argaddr(3, &un);
    return semctl((int)(semid), (int)(semnum), (int)(cmd), un);
}

uint64
sys_msgget(void)
{
    uint64 key;
    uint64 maxlen;
    uint64 flag;
    argaddr(0, &key);
    argaddr(1, &maxlen);
    argaddr(2, &flag);
    return msgget(key, (uint32)maxlen, (uint32)(flag));
}

uint64
sys_msgsnd(void)
{
    uint64 msqid;
    uint64 msgp;
    uint64 msgflg;
    argaddr(0, &msqid);
    argaddr(1, &msgp);
    argaddr(2, &msgflg);
    return msgsnd((int)msqid, (struct msgbuf*)(msgp), (int)(msgflg));
}

uint64
sys_msgrcv(void)
{
    uint64 msqid, msgp, msgsz, msgtype, msgflg;
    argaddr(0, &msqid);
    argaddr(1, &msgp);
    argaddr(2, &msgsz);
    argaddr(3, &msgtype);
    argaddr(4, &msgflg);
    return msgrcv((int)msqid, (struct msgbuf*)msgp, (uint32)msgsz, (uint32)msgtype, (int)msgflg);
}

uint64
sys_msgctl(void)
{
    uint64 msqid;
    uint64 cmd;
    uint64 buf;
    argaddr(0, &msqid);
    argaddr(1, &cmd);
    argaddr(2, &buf);
    return msgctl((ipc_id)msqid, (int)(cmd), (struct msqid_ds*)(buf));
}