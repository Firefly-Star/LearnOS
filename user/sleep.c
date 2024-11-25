#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{

  if(argc < 2){
    fprintf(2, "Usage: sleep number\n");
    exit(1);
  }else if (argc != 2){
    fprintf(2, "Too many args!! Usage: sleep number\n");
    exit(1);
  }
  
  char *tick_str = argv[1];
  for (int i=0; tick_str[i]!= '\0'; i++){
    if (tick_str[i] < '0' || tick_str[i] > '9'){
      fprintf(2, "Error! %s is not a integer\n", tick_str);
      exit(1);
    }
  }
  sleep(atoi(tick_str));
  exit(0);
}
