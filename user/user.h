#include "kernel/types.h"
#include "kernel/semaphore.h"
#include "kernel/shm.h"
struct stat;
struct timeVal;

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
int sleep(int); // added by LC
int trace(uint64); // mask of syscall to trace.
void sem_init(struct sem_t* sem, char* name, uint value); // 私有信号量的初始化
int shmget(key_t key, uint64 size, int shmflg);
void* shmat(ipc_id shmid, const void* shmaddr, int shmflag);
void shmdt(void* shmaddr);
int shmctl(int shmid, int cmd, struct shmid_ds *buf);
int vfork(void);
int semget(key_t key, uint64 size, uint flag);
int semop(int semid, struct sembuf* sops, uint nsops);
int semctl(int semid, int semnum, int cmd, uint64 un);

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

// usem.c
void sem_wait(struct sem_t*);