#ifndef TIME_H
#define TIME_H

#include "types.h"

struct timeVal
{
    uint64 sec; // sec since 
    uint64 usec; // number of microseconds passed in this second
};

struct timeVal _gettimeofday(void);

#endif