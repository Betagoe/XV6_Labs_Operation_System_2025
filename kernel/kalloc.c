// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#define PA2INDEX(pa) (((uint64)pa)/PGSIZE)
int cow_cnt[PHYSTOP/PGSIZE];

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    // 初始化时再将每一页的引用计数设为1
    cow_cnt[PA2INDEX(p)] = 1;
    kfree(p);
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // 需要加锁保证原子性
  acquire(&kmem.lock);
  int remain = --cow_cnt[PA2INDEX(pa)];
  release(&kmem.lock);

  if (remain > 0) {
    // 只有最后1个reference被删除时需要真正释放这个物理页
    return;
  }

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    int idx = PA2INDEX(r);
    if (cow_cnt[idx] != 0) {
      panic("kalloc: this page has been allocated before");
    }
    cow_cnt[idx] = 1; // 新allocate的物理页的计数器为1
  }
  return (void*)r;
}

// 对于子进程，需要一个函数来增加引用计数
void add_ref(uint64 pa, int num) {
    acquire(&kmem.lock);
    cow_cnt[PA2INDEX(pa)] += num;
    release(&kmem.lock);
}