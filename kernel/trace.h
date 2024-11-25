#ifndef TRACE_H
#define TRACE_H

#include "types.h"

// trace.c
#define TRACE_MASK(nsyscall) (1 << nsyscall)
void _trace(uint64);

#endif