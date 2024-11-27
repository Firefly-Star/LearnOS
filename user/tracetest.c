#include "user.h"
#include "kernel/trace.h"
#include "kernel/time.h"
int main()
{
    trace(TRACE_MASK(21) | TRACE_MASK(22) | TRACE_MASK(23));
    if(fork() == 0)
    {
        printf("%d\n", getyear());
    }
    else
    {
        sleep(1);
        struct timeVal t;
        gettimeofday(&t);
        printf("%lu , %lu\n", t.sec, t.usec);
    }
    exit(0);
}