#include "trace.h"
#include "proc.h"

extern struct proc* myproc(void);

void _trace(uint64 mask)
{
    struct proc* p = myproc();
    p->traceMask = mask;
}