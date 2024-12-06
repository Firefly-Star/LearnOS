#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int main()
{
    char* old = sbrk(0);
    printf("%lu", (uint64)(old));
    exit(0);
}