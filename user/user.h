#include "kernel/types.h"
#include "kernel/semaphore.h"
#include "kernel/shm.h"
#include "kernel/msg.h"
struct stat;
struct timeVal;

#define NULL 0

#define USER_ASSERT(x, info)\
do\
{\
    if (!(x))\
    {\
        printf("user panic: %s\n", info);\
        exit(-1);\
    }\
} while(0);


// system calls
int fork(void);
int exit(int) __attribute__((noreturn));
int wait(int*);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(const char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
int getyear(void);
int gettimeofday(struct timeVal*);
int sleep(int); // 睡眠一段时间
int trace(uint64); // mask of syscall to trace.
int set_priority(int, int); // 设置进程优先级
void printMlfq(void); // 输出多级反馈队列的运行情况
void chrt(int); // 切换调度算法
uint ticks();
uint64 cycles();
int shmget(key_t key, uint64 size, int shmflg);
void* shmat(ipc_id shmid, const void* shmaddr, int shmflag);
void shmdt(void* shmaddr);
int shmctl(int shmid, int cmd, struct shmid_ds *buf);
int vfork(void);
int semget(key_t key, uint64 size, uint flag);
int semop(int semid, struct sembuf* sops, uint nsops);
int semctl(int semid, int semnum, int cmd, union semun un);
int msgget(key_t key, uint32 maxlen, uint32 flag);
int msgsnd(ipc_id msqid, const struct msgbuf* msgp, int msgflg);
int msgrcv(ipc_id msqid, struct msgbuf* msgp, uint32 msgsz, uint32 msgtype, int msgflg);
int msgctl(ipc_id msqid, int cmd, struct msqid_ds* buf);
void neofetch(void);

// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void fprintf(int, const char*, ...) __attribute__ ((format (printf, 2, 3)));
void printf(const char*, ...) __attribute__ ((format (printf, 1, 2)));
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
int atoi(const char*);
int memcmp(const void *, const void *, uint);
void *memcpy(void *, const void *, uint);

// umalloc.c
void* malloc(uint);
void free(void*);

// find.c
void find(char*, char*); 

// mutex.c
typedef ipc_id mutex_id;
mutex_id init_mutex(key_t key);
int acquire_mutex(mutex_id mutexid);
int release_mutex(mutex_id mutexid);
void destruct_mutex(mutex_id mutexid);