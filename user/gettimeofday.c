#include "kernel/time.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main()
{
    struct timeVal currentTime;
    gettimeofday(&currentTime);
    printf("current sec: %lu, current usec: %lu\n", currentTime.sec, currentTime.usec);
    exit(0);
}