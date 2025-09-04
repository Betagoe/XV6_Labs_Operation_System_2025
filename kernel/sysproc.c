#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  //加入对backtrace的调用
  backtrace();
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// 用于开始alarm的系统调用
uint64
sys_sigalarm(void) {
  // 获取到当前进程
  struct proc *my_proc = myproc();
  // 从用户态获取参数
  int tick;
  if (argint(0, &tick) < 0)
    return -1;
  uint64 func;
  if(argaddr(1, &func) < 0)
    return -1;
  // 初始化alarm相关变量
  my_proc->alarm_tick = tick;
  my_proc->alarm_handler = (void (*)()) func;
  my_proc->alarm_interval = 0;
  return 0;
}

// 用于结束alarm的系统调用
uint64
sys_sigreturn(void) {
  struct proc* p = myproc();
  if (p->isalarm) {
    // 结束调用，返回现场
    p->isalarm = 0;
    *p->trapframe = *p->alarmframe;
  }
  return 0;
}