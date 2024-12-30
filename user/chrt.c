#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{ 

    if (argc!=2){
        printf("useage: chrt <schedule method>\n");
        exit(0);
    }

    if (strcmp(argv[1], "SCHED_FCFS") == 0) {
        chrt(0);
    }else if (strcmp(argv[1], "SCHED_RR") == 0) {
        chrt(1);
    }else if (strcmp(argv[1], "SCHED_MLFQ") == 0){
        chrt(2);
    }else if (strcmp(argv[1], "-c") == 0){
        chrt(-1);
    }else{
        printf("useage: chrt <schedule method>\n");
        exit(0);
    }

    exit(0);
}