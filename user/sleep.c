#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int 
main(int argc, char *argv[])
{
  int sleep_duration;
  if (argc < 2) {
    fprintf(2, "错误！请输入睡眠时间。\n");
    exit(1);
  }else if(argc > 2){
    fprintf(2, "错误！参数过多。\n");
    exit(1);
  }
  sleep_duration = atoi(argv[1]);
  sleep(sleep_duration);
  fprintf(1, "睡眠结束。\n");
  exit(0);
}