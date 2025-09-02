#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void 
prime_filter(int left_pipe[2])
{
    int first_data;
    if (read(left_pipe[0],&first_data,sizeof(int)) != sizeof(int)){
        exit(0);
    }
    // 读取到数据
    printf("prime %d\n",first_data);
    // 实现素数筛
    int right_pipe[2];
    pipe(right_pipe);
    int data;
    if(fork() == 0){
        // 子进程
        close(right_pipe[1]);
        prime_filter(right_pipe);
    }else{
        // 父进程
        while (read(left_pipe[0],&data,sizeof(int)) == sizeof(int)){
            if(data % first_data != 0){
                write(right_pipe[1],&data,sizeof(int));
            }
        }
        close(left_pipe[0]);
        close(right_pipe[1]);
        // 回收子进程
        wait(0);
    }
    exit(0);
}

// 使用素数筛进行判断
int
main(int argc, char const *argv[])
{
    int left_pipe[2];
    pipe(left_pipe);
    for (int i = 2; i <= 35; i++) {
        write(left_pipe[1], &i, sizeof(int));
    }
    close(left_pipe[1]);
    prime_filter(left_pipe);
    exit(0);
}