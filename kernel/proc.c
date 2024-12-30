#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "uvapg.h"
#include "semaphore.h"

struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

int nextpid = 1;
struct spinlock pid_lock;

extern void forkret(void);
static void freeproc(struct proc *p);

extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

enum schemethod SCHE_METHOD = SCHED_MLFQ;

// 用于实现多级反馈队列调度算法
struct MultiLevelFeedbackQueue
{
  int priority;
  struct procQueueNode* head;
  struct procQueueNode* tail;
  int runtime;
};

struct MultiLevelFeedbackQueue mlfqueue[NMLFQ];

void initMlfq()
{
    for (int i = 0; i < NMLFQ; i++)
    {
      mlfqueue[i].priority = 2*i+1;
      mlfqueue[i].head = NULL;
      mlfqueue[i].tail = mlfqueue[i].head;
      mlfqueue[i].runtime = i+1;
    }
}

int insertMlfq(struct proc* p){
    struct procQueueNode* node = (struct procQueueNode*)kmalloc(sizeof(struct procQueueNode));
    node->p = p;
    node->next = NULL;
    node->want = 0;

    struct MultiLevelFeedbackQueue* q;
    for (q = mlfqueue; q<&mlfqueue[NMLFQ]; q++){
      if (p->priority <= q->priority){
        if (q->head == NULL){
          q->head = node;
          q->tail = node;
        }
        else{
          q->tail->next = node;
          q->tail = node;
        }
        p->mlqlevel = q - mlfqueue;
        p->runtime = q->runtime;
        return q - mlfqueue;
      }
    }
    printf("error: proc %d priority %d too high.\n", p->pid, p->priority);
    return -1;
}

int insertNextMlfqTail(struct proc* p){
  struct procQueueNode* node = (struct procQueueNode*)kmalloc(sizeof(struct procQueueNode));
  node->p = p;
  node->next = NULL;
  node->want = 0;

  if (p->mlqlevel < NMLFQ-1){
    p->mlqlevel++;
  }

  if (mlfqueue[p->mlqlevel].head == NULL){
    mlfqueue[p->mlqlevel].head = node;
    mlfqueue[p->mlqlevel].tail = node;
  }
  else{
    mlfqueue[p->mlqlevel].tail->next = node;
    mlfqueue[p->mlqlevel].tail = node;
  }
  p->runtime = mlfqueue[p->mlqlevel].runtime;
  return p->mlqlevel;
}

void removeMlfq(struct proc *p){
  struct procQueueNode* node = mlfqueue[p->mlqlevel].head;

  if(node->p == p && node->next == NULL){ // 仅有一个节点
    mlfqueue[p->mlqlevel].head = NULL;
    mlfqueue[p->mlqlevel].tail = NULL;
    return;
  }else if(node->p == p) { // 头节点
    mlfqueue[p->mlqlevel].head = node->next;
    kmfree(node, sizeof(struct procQueueNode));
    return;
  }

  while(node->next!=NULL){
   if(node->next->p == p){
    struct procQueueNode* temp = node->next;
    node->next = node->next->next;
    if(mlfqueue[p->mlqlevel].tail == temp){
      mlfqueue[p->mlqlevel].tail = node;
    }
    kmfree(temp, sizeof(struct procQueueNode));
    return;
   }
   node = node->next;
  }
  
  printf("error: proc %d not in mlqueue.", p->pid);
  return; 
}

// test function
void printMlfq(){
  struct MultiLevelFeedbackQueue *q;
  printf("\n");
  for (q=mlfqueue; q<&mlfqueue[NMLFQ]; q++){
    struct procQueueNode *node = q->head;
    while(node!=NULL){
      if (node->p->pid == 1 || node->p->pid == 2){
        node = node->next;
        continue;
      }
      if (node->next != NULL){
        printf("pid: %d, state: %d, priority: %d, mlqlevel: %d, runtime: %d, nextpid: %d\n", node->p->pid, node->p->state, node->p->priority, node->p->mlqlevel, node->p->runtime, node->next->p->pid);
      }else {
        printf("pid: %d, state: %d, priority: %d, mlqlevel: %d, runtime: %d\n", node->p->pid, node->p->state, node->p->priority, node->p->mlqlevel, node->p->runtime);
      }
      node = node->next;
    }
  }
}

// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
void
proc_mapstacks(pagetable_t kpgtbl)
{
  struct proc *p;
  
  for(p = proc; p < &proc[NPROC]; p++) {
    char *pa = kalloc();
    if(pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int) (p - proc));
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }
}

// initialize the proc table.
void
procinit(void)
{
  struct proc *p;
  
  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  for(p = proc; p < &proc[NPROC]; p++) {
      initlock(&p->lock, "proc");
      p->state = UNUSED;
      p->kstack = KSTACK((int) (p - proc));
      p->traceMask = 0;
      p->freeva_head = (struct freeva*)(kmalloc(sizeof(struct freeva)));
      init_freeva(p->freeva_head);
      init_procshmblock(&p->proc_shmhead);
      init_procsemblock(&p->proc_semhead);
      init_procmsgblock(&p->proc_msghead);
      p->vforked = 0;
  }
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int
cpuid()
{
  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu*
mycpu(void)
{
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc*
myproc(void)
{
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}

int
allocpid()
{
  int pid;
  
  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);

  return pid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();
  p->state = USED;

  p->priority = 1;
  p->preempable = 1;
  p->createtime = ticks;
  p->runtime = 1;
  p->readytime = 0;
  p->sleeptime = 0;
  p->nice = 0;

  // Allocate a trapframe page.
  if((p->trapframe = (struct trapframe *)kalloc()) == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if(p->pagetable == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;

  return p;
}

static struct proc*
allocproc_v(void)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();
  p->state = USED;

  p->priority = 1;
  p->preempable = 1;
  p->createtime = ticks;
  p->runtime = 1;
  p->readytime = 0;
  p->sleeptime = 0;
  p->nice = 0;

  // vfork的进程有自己的陷阱帧
  if((p->trapframe = (struct trapframe *)kalloc()) == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // vfork的进程不需要自己的页表

  // vfork的进程有自己的cpu的context
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;

  return p;
}

// 分离进程中所有的共享内存
void
proc_freeshm(struct proc* p)
{
    while(p->proc_shmhead.next != NULL)
    {
        shmdt(p, p->proc_shmhead.next->va);        
    }
}

void
proc_freesem(struct proc* p)
{
    while(p->proc_semhead.next != NULL)
    {
        struct proc_semblock* toremove = p->proc_semhead.next;
        p->proc_semhead.next = toremove->next;
        acquire(&toremove->sem->lk);
        toremove->sem->ref_count -= 1;
        if (toremove->sem->state == IPC_ZOMBIE && toremove->sem->ref_count == 0)
        {
            sem_reinit(toremove->sem);
        }
        release(&toremove->sem->lk);
        kmfree(toremove, sizeof(struct proc_semblock));
    }
}

void
proc_freemsq(struct proc* p)
{
    while(p->proc_msghead.next != NULL)
    {
        struct proc_msgblock* toremove = p->proc_msghead.next;
        p->proc_msghead.next = toremove->next;
        acquire(&toremove->msq->lk);
        toremove->msq->ref_count -= 1;
        if (toremove->msq->state == IPC_ZOMBIE && toremove->msq->ref_count == 0)
        {
            msq_reinit(toremove->msq);
        }
        release(&toremove->msq->lk);
        kmfree(toremove, sizeof(struct proc_semblock));
    }
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
  if(p->trapframe)
    kfree((void*)p->trapframe);
  p->trapframe = 0;
  if(p->pagetable)
  {
    proc_freesem(p);
    proc_freemsq(p);
    if (!p->vforked)
    { // 非vfork出来的进程
        // 清理共享内存引用计数
        // 清理堆内存
        proc_freeshm(p);
        proc_freepagetable(p->pagetable, p->sz);
    }
  }

  if(SCHE_METHOD == SCHED_MLFQ){
    removeMlfq(p);
  }

  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
  p->traceMask = 0;

  p->priority = 0;
  p->createtime = 0;
  p->runtime = 0;
  p->readytime = 0;
  p->sleeptime = 0;
  p->nice = 0;

  init_freeva(p->freeva_head);
  p->vforked = 0;
  p->sem_want = 0;
}

// Create a user page table for a given process, with no user memory,
// but with trampoline and trapframe pages.
pagetable_t
proc_pagetable(struct proc *p)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if(pagetable == 0)
    return 0;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if(mappages(pagetable, TRAMPOLINE, PGSIZE,
              (uint64)trampoline, PTE_R | PTE_X) < 0){
    uvmfree(pagetable, 0);
    return 0;
  }

  // map the trapframe page just below the trampoline page, for
  // trampoline.S.
  if(mappages(pagetable, TRAPFRAME, PGSIZE,
              (uint64)(p->trapframe), PTE_R | PTE_W) < 0){
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void
proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  uvmunmap(pagetable, TRAPFRAME, 1, 0);

  uvmfree(pagetable, sz);
}

// a user program that calls exec("/init")
// assembled from ../user/initcode.S
// od -t xC ../user/initcode
uchar initcode[] = {
  0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
  0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
  0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
  0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
  0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
  0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// Set up first user process.
void
userinit(void)
{
  struct proc *p;

  p = allocproc();
  initproc = p;
  
  // allocate one user page and copy initcode's instructions
  // and data into it.
  uvmfirst(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;

  // prepare for the very first "return" from kernel to user.
  p->trapframe->epc = 0;      // user program counter
  p->trapframe->sp = PGSIZE;  // user stack pointer

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;

  insertMlfq(p);

  release(&p->lock);
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint64 sz;
  struct proc *p = myproc();

  sz = p->sz;
  if(n > 0){
    if((sz = uvmalloc(p->pagetable, sz, sz + n, PTE_W)) == 0) {
      return -1;
    }
  } else if(n < 0){
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->sz = sz;
  return 0;
}

void copy_sem(struct proc* old, struct proc* new)
{
    struct proc_semblock* workptr = old->proc_semhead.next;
    while (workptr != NULL)
    {
        struct proc_semblock* newnode = (struct proc_semblock*)(kmalloc(sizeof(struct proc_semblock)));
        newnode->flag = workptr->flag;
        newnode->sem = workptr->sem;
        acquire(&workptr->sem->lk);
        workptr->sem->ref_count += 1;
        release(&workptr->sem->lk);
        newnode->next = new->proc_semhead.next;
        new->proc_semhead.next = newnode;
        workptr = workptr->next;
    }
}

void copy_msg(struct proc* old, struct proc* new)
{
    struct proc_msgblock* workptr = old->proc_msghead.next;
    while (workptr != NULL)
    {
        struct proc_msgblock* newnode = (struct proc_msgblock*)(kmalloc(sizeof(struct proc_msgblock)));
        newnode->flag = workptr->flag;
        newnode->msq = workptr->msq;
        acquire(&workptr->msq->lk);
        workptr->msq->ref_count += 1;
        release(&workptr->msq->lk);
        newnode->next = new->proc_msghead.next;
        new->proc_msghead.next = newnode;
        workptr = workptr->next;
    }
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *p = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  np->traceMask = p->traceMask;

  // Copy user memory from parent to child.
  if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0){
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  uvmcopy_shm(p, np);
  copy_sem(p, np);
  copy_msg(p, np);
  np->sz = p->sz;

  // copy saved user registers.
  *(np->trapframe) = *(p->trapframe);

  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;

  // increment reference counts on open file descriptors.
  for(i = 0; i < NOFILE; i++)
    if(p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  release(&np->lock);

  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  acquire(&np->lock);
  np->state = RUNNABLE;
  release(&np->lock);

  acquire(&np->lock);
  np->priority = p->priority;
  release(&np->lock);

  if(SCHE_METHOD == SCHED_MLFQ){
    insertMlfq(np);
  }

  return pid;
}

// 阻塞父进程，直至子进程exit或者exec
// 子进程共享父进程的内存空间，即共用同一个页表
// 因此子进程对堆栈的所有操作都会反映在父进程中
int vfork()
{
    int i, pid;
    struct proc *np;
    struct proc *p = myproc();

    // Allocate process.
    if((np = allocproc_v()) == 0){
        return -1;
    }

    np->traceMask = p->traceMask;
    np->vforked = 1;

    // 接管使用父进程的页表
    np->pagetable = p->pagetable;

    // 将TRAPFRAME位置的虚拟内存所映射的内存页替换为自己的trapframe
    uvmunmap(np->pagetable, TRAPFRAME, 1, 0);
    mappages(np->pagetable, TRAPFRAME, PGSIZE, (uint64)(np->trapframe), PTE_R | PTE_W);

    // 接管父进程的shm块, sem块, msg块
    np->proc_shmhead.next = p->proc_shmhead.next;
    np->proc_semhead.next = p->proc_semhead.next;
    np->proc_msghead.next = p->proc_msghead.next;

    // 接管父进程的shm地址管理块
    np->freeva_head->next = p->freeva_head->next;
    np->sz = p->sz;

    // copy saved user registers.
    *(np->trapframe) = *(p->trapframe);

    // Cause fork to return 0 in the child.
    np->trapframe->a0 = 0;

    // increment reference counts on open file descriptors.
    for(i = 0; i < NOFILE; i++)
        if(p->ofile[i])
        np->ofile[i] = filedup(p->ofile[i]);
    np->cwd = idup(p->cwd);

    safestrcpy(np->name, p->name, sizeof(p->name));

    pid = np->pid;

    release(&np->lock);

    acquire(&wait_lock);
    np->parent = p;
    release(&wait_lock);

    acquire(&np->lock);
    np->state = RUNNABLE;
    release(&np->lock);

    acquire(&np->lock);
    np->priority = p->priority;
    release(&np->lock);

    if(SCHE_METHOD == SCHED_MLFQ){
      insertMlfq(np);
    }

    acquire(&wait_lock);
    sleep(p, &wait_lock);
    release(&wait_lock);

    return pid;
}

// Pass p's abandoned children to init.
// Caller must hold wait_lock. TODO: 在proc中加入child字段，让reparent不再需要扫描所有的proc
void
reparent(struct proc *p)
{
  struct proc *pp;

  for(pp = proc; pp < &proc[NPROC]; pp++){
    if(pp->parent == p){
      pp->parent = initproc;
      wakeup(initproc);
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void
exit(int status)
{
  struct proc *p = myproc();

  if(p == initproc)
    panic("init exiting");

  // Close all open files.
  for(int fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd]){
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(p->cwd);
  end_op();
  p->cwd = 0;

  acquire(&wait_lock);

  // Give any children to init.
  reparent(p);

  // 如果是vfork出来的进程
  if(p->vforked)
  {
    // 归还shm, sem, msg管理块和虚拟地址管理块
    p->parent->proc_shmhead.next = p->proc_shmhead.next;
    p->parent->proc_semhead.next = p->proc_semhead.next;
    p->parent->proc_msghead.next = p->proc_msghead.next;

    p->proc_shmhead.next = NULL;
    p->proc_semhead.next = NULL;
    p->proc_msghead.next = NULL;

    p->parent->freeva_head->next = p->freeva_head->next;
    p->freeva_head->next = NULL;
    
    // 归还trapframe
    uvmunmap(p->pagetable, TRAPFRAME, 1, 0);
    mappages(p->pagetable, TRAPFRAME, PGSIZE, (uint64)(p->parent->trapframe), PTE_W | PTE_R);
  }

  // Parent might be sleeping in wait().
  wakeup(p->parent);
  
  acquire(&p->lock);

  p->xstate = status;
  p->state = ZOMBIE;

  release(&wait_lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(uint64 addr)
{
  struct proc *pp;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(pp = proc; pp < &proc[NPROC]; pp++){
      if(pp->parent == p){
        // make sure the child isn't still in exit() or swtch().
        acquire(&pp->lock);

        havekids = 1;
        if(pp->state == ZOMBIE){
          // Found one.
          pid = pp->pid;
          if(addr != 0 && copyout(p->pagetable, addr, (char *)&pp->xstate,
                                  sizeof(pp->xstate)) < 0) {
            release(&pp->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(pp);
          release(&pp->lock);
          release(&wait_lock);
          return pid;
        }
        release(&pp->lock);
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || killed(p)){
      release(&wait_lock);
      return -1;
    }
    
    // Wait for a child to exit.
    sleep(p, &wait_lock);  //DOC: wait-sleep
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();

  c->proc = 0;
  for(;;){
    // The most recent process to run may have had interrupts
    // turned off; enable them to avoid a deadlock if all
    // processes are waiting.
    intr_on();
    // struct proc *pmax = 0;
    int found = 0;

    switch(SCHE_METHOD){
      case SCHED_FCFS:
        goto fcfs;
        break;
      case SCHED_RR:
        goto rr;
        break;
      case SCHED_MLFQ:
        goto mlfq;
        break;
      default:
        panic("scheduler: unknown scheduling method");
    }
    fcfs:{
      panic("TODO: fcfs\n");
    }
    rr:{
      for(p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);
        if(p->state == RUNNABLE) {

          p->state = RUNNING;
          c->proc = p;
          swtch(&c->context, &p->context);

          c->proc = 0;
          found = 1;
        }
        release(&p->lock);
      }
      if(!found) {
        // nothing to run; stop running on this core until an interrupt.
        intr_on();
        asm volatile("wfi");
      }
    }
    mlfq:{
      for (int i = 0; i < NMLFQ; i++){
        struct procQueueNode *node = mlfqueue[i].head;
        while (node != NULL)
        {
          p = node->p;
          struct procQueueNode* next = node->next; // 防止更新队列后丢失下一个节点
          acquire(&p->lock);
          if(p->state == RUNNABLE){
            found=1;
            p->state = RUNNING;
            c->proc = p;
            // printf("pid %d, priority %d, mlqlevel %d, runtime %d\n", p->pid, p->priority, p->mlqlevel, p->runtime);
            swtch(&c->context, &p->context);
            c->proc = 0;
            release(&p->lock);
          }else{
            release(&p->lock);
          }
          node = next;
        }
      }
      if (!found) {
        // nothing to run; stop running on this core until an interrupt.
        intr_on();
        asm volatile("wfi");
      }
    }

  }
}


// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&p->lock))
    panic("sched p->lock");
  if(mycpu()->noff != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
  if (SCHE_METHOD == SCHED_RR){
    p->runtime = 1;
  }
  else if (SCHE_METHOD == SCHED_MLFQ){
    removeMlfq(p);
    insertNextMlfqTail(p);
    // printMlfq();
  }
  sched();
  release(&p->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void
forkret(void)
{
  static int first = 1;

  // Still holding p->lock from scheduler.
  release(&myproc()->lock);

  if (first) {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    fsinit(ROOTDEV);

    first = 0;
    // ensure other cores see first=0.
    __sync_synchronize();
  }

  usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.

  acquire(&p->lock);  //DOC: sleeplock1
  release(lk);

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  release(&p->lock);
  acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void
wakeup(void *chan)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    if(p != myproc()){
      acquire(&p->lock);
      if(p->state == SLEEPING && p->chan == chan) {
        p->state = RUNNABLE;
      }
      release(&p->lock);
    }
  }
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int
kill(int pid)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid){
      p->killed = 1;
      if(p->state == SLEEPING){
        // Wake process from sleep().
        p->state = RUNNABLE;
      }
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

void
setkilled(struct proc *p)
{
  acquire(&p->lock);
  p->killed = 1;
  release(&p->lock);
}

int
killed(struct proc *p)
{
  int k;
  
  acquire(&p->lock);
  k = p->killed;
  release(&p->lock);
  return k;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int
either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
  struct proc *p = myproc();
  if(user_dst){
    return copyout(p->pagetable, dst, src, len);
  } else {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int
either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
  struct proc *p = myproc();
  if(user_src){
    return copyin(p->pagetable, dst, src, len);
  } else {
    memmove(dst, (char*)src, len);
    return 0;
  }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [USED]      "used",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runable",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie",
  [BLOCKED]   "blocked",
  };
  struct proc *p;
  char *state;

  printf("\n");
  for(p = proc; p < &proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    printf("%d %s %s, stacktop: %lx, memsz: %lx", p->pid, state, p->name, p->ustack_top, p->sz);
    printf("\n");
  }
}

// set the priority of a process
int set_priority(int pid, int priority)
{
    struct proc *p=0;
    // for (p = proc; p < &proc[NPROC]; p++) {
    //     if (p->pid == pid) {
    //       acquire(&p->lock);
    //       p->priority = priority;
    //       release(&p->lock);
    //       return 0;
    //     }
    // }
    if (SCHE_METHOD == SCHED_RR){
      for (p = proc; p < &proc[NPROC]; p++) {
        if (p->pid == pid) {
          acquire(&p->lock);
          p->priority = priority;
          release(&p->lock);
          return 0;
        }
      }
    }else if(SCHE_METHOD == SCHED_MLFQ){
      struct MultiLevelFeedbackQueue* q;
      for (q = mlfqueue; q<&mlfqueue[NMLFQ]; q++){
        struct procQueueNode* node = q->head;
        while(node!=NULL){
          p = node->p;
          if(p->pid == pid){
            acquire(&p->lock);
            removeMlfq(p);
            p->priority = priority;
            insertMlfq(p);
            release(&p->lock);
            return 0;
          }
          node = node->next;
        }
      }
    }

    printf("cannot find process %d\n", pid);
    return -1;
}

int nice(){
  // TODO:
  return 0;
}

// 切换进程调度方案
int chrt(int type){
  switch (type) {
    case SCHED_FCFS:  // 0
      SCHE_METHOD = SCHED_FCFS;
      break;
    case SCHED_RR:  // 1
      SCHE_METHOD = SCHED_RR;
      break;
    case SCHED_MLFQ:  // 2
      SCHE_METHOD = SCHED_MLFQ;
      break;
    case -1:
      switch(SCHE_METHOD){
        case SCHED_FCFS:
          printf("current scheduling method: SCHED_FCFS\n");
          break;
        case SCHED_RR:
          printf("current scheduling method: SCHED_RR\n");
          break;
        case SCHED_MLFQ:
          printf("current scheduling method: SCHED_MLFQ\n");
          break;
        default:
          break;
        }
      break;
    default:
      printf("unkown scheduling method\n");
      return -1;
  }
  return 0;
}

void update_proc(){
  struct proc *p;
  for (p=proc; p<&proc[NPROC]; p++){
    acquire(&p->lock);
    int found = 0;
    switch(p->state){
      case RUNNING:
        p->runtime++;
        found=1;
        break;
      case SLEEPING:
        p->sleeptime++;
        found = 1;
        break;
      case RUNNABLE:
        p->readytime++;
        found=1;
        break;
      default:
        break;
    }
    if (found){
      p->nice = p->priority - p->runtime - p->sleeptime + p->readytime;
    }
    release(&p->lock);
  }
} 

void neofetch(){
  printf("Welcome to the SupremeOS!\n");

  printf("\n");
  printf("  █████████                                                                         ███████     █████████ \n");
  printf(" ███░░░░░███                                                                      ███░░░░░███  ███░░░░░███\n");
  printf("░███    ░░░  █████ ████ ████████  ████████   ██████  █████████████    ██████     ███     ░░███░███    ░░░ \n");
  printf("░░█████████ ░░███ ░███ ░░███░░███░░███░░███ ███░░███░░███░░███░░███  ███░░███   ░███      ░███░░█████████ \n");
  printf(" ░░░░░░░░███ ░███ ░███  ░███ ░███ ░███ ░░░ ░███████  ░███ ░███ ░███ ░███████    ░███      ░███ ░░░░░░░░███\n");
  printf(" ███    ░███ ░███ ░███  ░███ ░███ ░███     ░███░░░   ░███ ░███ ░███ ░███░░░     ░░███     ███  ███    ░███\n");
  printf("░░█████████  ░░████████ ░███████  █████    ░░██████  █████░███ █████░░██████     ░░░███████░  ░░█████████ \n");
  printf(" ░░░░░░░░░    ░░░░░░░░  ░███░░░  ░░░░░      ░░░░░░  ░░░░░ ░░░ ░░░░░  ░░░░░░        ░░░░░░░     ░░░░░░░░░  \n");
  printf("                        ░███                                                                              \n");
  printf("                        █████                                                                             \n");
  printf("                       ░░░░░                                                                              \n");
  printf("\n");

  printf("The version is: %s\n", "0.1");
  printf("Made by: %s, %s\n", "wxz", "lc");

  printf("Basic information:\n");
  switch(SCHE_METHOD){
  case SCHED_FCFS:
    printf("current scheduling method: SCHED_FCFS\n");
    break;
  case SCHED_RR:
    printf("current scheduling method: SCHED_RR\n");
    break;
  case SCHED_MLFQ:
    printf("current scheduling method: SCHED_MLFQ\n");
    break;
  default:
    break;
  }

  // TODO:
  
}