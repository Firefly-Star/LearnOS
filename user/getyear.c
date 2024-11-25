#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

int
main(void)
{
    printf("Note: Unix v6 was released in year %d\n", getyear());
    exit(0);
}