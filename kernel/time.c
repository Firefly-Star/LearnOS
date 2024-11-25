#include "time.h"
#include "spinlock.h"

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