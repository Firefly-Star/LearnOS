#include "time.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71


struct timeVal _gettimeofday(void)
{
    struct timeVal a;

    // TODO: initialize RTC
    a.sec = 100;
    a.usec = 1000;
    return a;
}

uint64 init_cycles[NCPU];

void timeinithart()
{
    init_cycles[cpuid()] = r_time();
}