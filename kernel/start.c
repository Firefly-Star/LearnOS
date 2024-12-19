#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

void main();
void timerinit();

// entry.S needs one stack per CPU.
__attribute__ ((aligned (16))) char stack0[4096 * NCPU]; // 为每个 CPU 分配 4096 字节的栈

// entry.S jumps here in machine mode on stack0.
void start() {
    // 设置 M Previous Privilege mode 为 Supervisor, 用于 mret。
    unsigned long x = r_mstatus();
    x &= ~MSTATUS_MPP_MASK; // 清除 MPP 位
    x |= MSTATUS_MPP_S;     // 设置为 Supervisor 模式
    w_mstatus(x);          // 写入 MSTATUS 寄存器

    // 设置 M Exception Program Counter 为 main，准备 mret。
    w_mepc((uint64)main);

    // 现在禁用分页。
    w_satp(0);

    // 将所有中断和异常委托给 Supervisor 模式。
    w_medeleg(0xffff);
    w_mideleg(0xffff);
    w_sie(r_sie() | SIE_SEIE | SIE_STIE | SIE_SSIE); // 使能 Supervisor 模式下的中断

    // 配置物理内存保护，允许 Supervisor 模式访问所有物理内存。
    w_pmpaddr0(0x3fffffffffffffull);
    w_pmpcfg0(0xf);

    // 请求时钟中断。
    timerinit();

    // 将每个 CPU 的 hartid 存储在其 tp 寄存器中，以供 cpuid() 使用。
    int id = r_mhartid();
    w_tp(id);

    // 切换到 Supervisor 模式并跳转到 main()。 
    asm volatile("mret");
}

// 请求每个 hart 生成定时器中断。
void timerinit() {
    // 启用 Supervisor 模式下的定时器中断。
    w_mie(r_mie() | MIE_STIE);
    
    // 启用 sstc 扩展（即 stimecmp）。让监管模式可以更好地管理定时器，包括设置定时器中断等，从而允许操作系统对时间事件进行处理。
    w_menvcfg(r_menvcfg() | (1L << 63)); 
    
    // 允许 Supervisor 使用 stimecmp 和 time。通过允许 Supervisor 使用 stimecmp 和 time，操作系统能够有效地利用计时器进行时间管理和事件调度。这意味着能更方便地实现定时器中断、调度等功能，以更好地支持多任务环境。
    w_mcounteren(r_mcounteren() | 2);
    
    // 请求第一次定时器中断。
    w_stimecmp(r_time() + 1000000);
}