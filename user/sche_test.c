#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{ 

    if (argc!=2){
        printf("useage: sche_test <forknumber>");
        exit(0);
    }
    int forknumber = atoi(argv[1]);

    for (int i=0; i<forknumber; i++){
        int pid = fork();
        
        if(pid){
            set_priority(pid, i+1);
        }
        else {
            for(int i=0; i<50; i++){
                printf("pid %d is runing\n", getpid());
                int result = 1;
                result = result + i;
                result = result - i;
                result = result * i;
                result = result / i;
            }
            exit(0);
        }
    }

    exit(0);
}