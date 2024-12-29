#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

volatile static int started = 0;

// start() jumps here in supervisor mode on all CPUs.
void
main()
{
    if(cpuid() == 0){
        consoleinit();
        printfinit();
        printf("\n");
        printf("customed xv6 kernel is booting\n");
        printf("satp: %lu, memory mapping not enabled in hart %d.\n", r_satp(), cpuid());
        printf("\n");
        kinit();         // physical page allocator
        kvminit();       // create kernel page table
        kvminithart();   // turn on paging
        slab_init();
        procinit();      // process table
        initMlq();       // 多级队列
        seminit();       // 信号量
        shminit();       // 共享内存
        msqinit();       // 消息队列
        trapinit();      // trap vectors
        trapinithart();  // install kernel trap vector
        plicinit();      // set up interrupt controller
        plicinithart();  // ask PLIC for device interrupts
        cyclesinithart();// 记录启动时的计时器周期数
        binit();         // buffer cache
        iinit();         // inode table
        fileinit();      // file table
        virtio_disk_init(); // emulated hard disk
        userinit();      // first user process
        __sync_synchronize();
        started = 1;
    } else {
        while(started == 0)
        ;
        __sync_synchronize();
        printf("hart %d starting\n", cpuid());
        printf("satp: %lu, memory mapping not enabled in hart %d.\n", r_satp(), cpuid());
        kvminithart();    // turn on paging
        trapinithart();   // install kernel trap vector
        plicinithart();   // ask PLIC for device interrupts
        cyclesinithart();  // 记录启动时的计时器周期数
    }
    scheduler();        
}
