#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{ 

    if (argc!=2){
        printf("useage: sche_test <forknumber>\n");
        exit(0);
    }
    int forknumber = atoi(argv[1]);

    for (int i=0; i<forknumber; i++){
        int pid = fork();
        if(pid){
            set_priority(pid, i+2);
            if (i==forknumber-1)
                printMlfq();
        }
        else {
            for(int i=0; i<100; i++){
                printf("pid %d is runing\n", getpid());
                int result = 1235;
                // 复杂计算 消耗时间
                for(int j=0; j<100000; j++){
                    result = result * 2;
                }
            }
            exit(0);
        }
    }
    wait(NULL);
    exit(0);
}