#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc,int *argv[])
{
    // 建立管道，并提前为子进程分配好读写端
    int pipe1[2];
    int pipe2[2];
    pipe(pipe1);
    pipe(pipe2);
    // 父子进程共用
    char buffer[4];
    if (fork() != 0){
        // 父进程
        write(pipe1[1],"ping",4);
        read(pipe2[0],buffer,4);
        printf("%d: received %s\n",getpid(),buffer);
        exit(0);
    } else {
        // 子进程
        read(pipe1[0],buffer,4);
        printf("%d: received %s\n",getpid(),buffer);
        write(pipe2[1],"pong",4);
        exit(0);
    }
}