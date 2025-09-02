#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int i;

  for(i = 1; i < argc; i++){
    write(1, argv[i], strlen(argv[i]));
    if(i + 1 < argc){
      // 字符串之间添加空格
      write(1, " ", 1);
    } else {
      // 最后一个字符串后面添加换行符
      write(1, "\n", 1);
    }
  }
  exit(0);
}
