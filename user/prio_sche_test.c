#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{ 
    int pid;
    printf("This is a demo for priority schedule\n");
    pid = getpid();
    set_priority(pid, 5); //系统默认优先级是 1

    int i  = 0;
           
    pid = fork();
    if(pid == 0) {
        set_priority(getpid(),5);
        i = 1;
        while(i <= 10)
        {
            printf("p2 is running\n");
            i++;
        }
        printf("p2 sleeping\n");
        sleep(100);
        i = 1;
        while(i <= 10)
        {
            printf("p2 is running again\n");
            i++;
        }
        printf("p2 finshed\n");
    } else {
        i = 1;
        while(i <= 10)
        {
            printf("p1 is running\n");
            i++;
        }
    }
    exit(1);
}