#include "user.h"

int main()
{
    volatile int y = 0;
    if(vfork())
    {
        printf("p: %d.\n", y);
    }
    else
    {
        y = 100;
        //printf("c: %d.\n", y);
    }
}