#include "user.h"
#include "kernel/trace.h"
#include "kernel/time.h"
int main()
{
    trace(~0);
    if(fork() == 0)
    {
        printf("%d\n", getyear());
    }
    else
    {
        sleep(1);
        struct timeVal t;
        gettimeofday(&t);
        printf("%lu , %lu", t.sec, t.usec);
    }
    exit(0);
}