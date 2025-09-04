// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

// 原来的缓存大小为 NBUF = 30
// 缓存哈希表的桶数和大小
#define BUCKETNUM	13
#define BUFSIZE		17
#define HASH(blockno)	(blockno % BUCKETNUM)

extern uint ticks; // system time clock

struct {
  struct spinlock lock;
  struct buf buf[BUFSIZE];
} bcachebucket[BUCKETNUM];

void
binit(void)
{
  // 初始化每个桶的锁和每个缓冲区的锁
  for (int i = 0; i < BUCKETNUM; i++) {
    initlock(&bcachebucket[i].lock, "bcachebucket");
    for (int j = 0; j < BUFSIZE; j++) {
      initsleeplock(&bcachebucket[i].buf[j].lock, "buffer");
    }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno){
  struct buf *b;

  acquire(&bcachebucket[HASH(blockno)].lock);

  // 判断是否缓存命中
  for(int i = 0; i < BUFSIZE; i++){
    b = &bcachebucket[HASH(blockno)].buf[i];
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      // 更新 LRU 时间戳
      b->LRU = ticks;
      release(&bcachebucket[HASH(blockno)].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  uint least = 0xffffffff; // 这个是最大的unsigned int
  int least_idx = -1;
  // 遍历桶内的缓冲区，找到 LRU 时间戳最小且未被使用的缓冲区
  for(int i = 0; i < BUFSIZE; i++){
    b = &bcachebucket[HASH(blockno)].buf[i];
    if(b->refcnt == 0 && b->LRU < least) {
      least = b->LRU;
      least_idx = i;
    }
  }
  if(least_idx == -1) {
    int isfind = 0;
    // // 开始从邻居桶中寻找
    // for (int i = 1; i < BUCKETNUM && !isfind; i++) {
    //   int neighbor_bucket = (HASH(blockno) + i) % BUCKETNUM;
    //   for(int j = 0; j < BUFSIZE && !isfind; j++){
    //     b = &bcachebucket[neighbor_bucket].buf[j];
    //     if(b->refcnt == 0 && b->LRU < least) {
    //       least = b->LRU;
    //       least_idx = j;
    //       // 找到后需要切换锁
    //       acquire(&bcachebucket[neighbor_bucket].lock);
    //       release(&bcachebucket[HASH(blockno)].lock);
    //       isfind = 1;
    //       break;
    //     }
    //   }
    // }
    if (!isfind) {
      // 所有缓冲区都在使用中
      release(&bcachebucket[HASH(blockno)].lock);
      panic("bget: no buffers");
    }
  }

  // 如果没有找到合适的缓冲区，说明所有缓冲区都在使用中

  // 找到缓冲区，更新信息并返回
  b = &bcachebucket[HASH(blockno)].buf[least_idx];
  b->dev = dev;
  b->blockno = blockno;
  b->valid = 0;
  b->refcnt = 1;
  // 更新 LRU 时间戳
  b->LRU = ticks;
  release(&bcachebucket[HASH(blockno)].lock);
  acquiresleep(&b->lock);
  return b;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");
  acquire(&bcachebucket[HASH(b->blockno)].lock);
  b->refcnt--;
  release(&bcachebucket[HASH(b->blockno)].lock);
  releasesleep(&b->lock);
}

void
bpin(struct buf *b) {
  acquire(&bcachebucket[HASH(b->blockno)].lock);
  b->refcnt++;
  release(&bcachebucket[HASH(b->blockno)].lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcachebucket[HASH(b->blockno)].lock);
  b->refcnt--;
  release(&bcachebucket[HASH(b->blockno)].lock);
}


