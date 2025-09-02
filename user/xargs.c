#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int
main(int argc, char *argv[])
{
    char buf[512];
    char *new_argv[MAXARG];
    int i, n = 0;

    // 将命令行参数复制到 new_argv 数组
    for (i = 1; i < argc; i++) {
        new_argv[n++] = argv[i];
    }

    // 读取标准输入的每一行
    while (1) {
        char *p = buf;
        // 找到字符串末尾
        while (read(0, p, 1) == 1 && *p != '\n') {
            // 检查是否遇到引号
            if (*p == '\"') {
                // 这里不要p++，通过后续输入覆盖掉引号
                continue;
            }
            // 检查是否遇到换行符
            if (*p == 'n' && *(p - 1) == '\\') {
                *(p - 1) = 0; // 将行末设置为字符串结束符
                p++;
                break;
            }
            p++;
        }
        
        if (p == buf) { // 如果没有读取到内容，退出
            break;
        }
        *p = 0; // 将行末设置为字符串结束符

        // 将读取的行作为参数添加到 new_argv 数组
        new_argv[n] = buf;
        new_argv[n + 1] = 0; // 确保 argv 以 NULL 结尾

        if (fork() == 0) {
            // 子进程执行命令
            exec(argv[1], new_argv);
        } else {
            // 父进程等待子进程完成
            wait(0);
        }
    }
    exit(0);
}