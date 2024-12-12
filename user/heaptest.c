#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int main()
{
    char* old = sbrk(1);
    printf("%lu", (uint64)(old));
    sleep(100);
    exit(0);
}