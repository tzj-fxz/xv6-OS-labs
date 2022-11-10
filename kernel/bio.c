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

#define NBUCKET 13

struct {
  struct spinlock globallock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct spinlock lock[NBUCKET];
  struct buf head[NBUCKET];
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.globallock, "bcache_global");

  // Create linked list of buffers
  for (int i = 0; i < NBUCKET; ++i){
    initlock(&bcache.lock[i], "bcache_hash");
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }
  // for (int i = 0; i < NBUF; ++i){
  int i = 0;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->tick_num = ticks;
    b->next = bcache.head[0].next;
    b->prev = &bcache.head[0];
    initsleeplock(&b->lock, "bcache_buffer");
    bcache.head[0].next->prev = b;
    bcache.head[0].next = b;
    ++i;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  uint hashnum = (dev * 8 + blockno) % NBUCKET;
  // printf("dev: %d, blockno: %d, hashnum: %d\n", dev, blockno, hashnum);

  acquire(&bcache.lock[hashnum]);

  // Is the block already cached?
  for(b = bcache.head[hashnum].next; b != &bcache.head[hashnum]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      b->tick_num = ticks;
      release(&bcache.lock[hashnum]);
      // printf("refcnt: %d\n", b->refcnt);
      acquiresleep(&b->lock);
      return b;
    }
  }
  // release(&bcache.lock[hashnum]);

  // Not cached.
  // Recycle the least recently used (b) unused buffer.
  acquire(&bcache.globallock);

  for (uint j = hashnum + 1; j < hashnum + NBUCKET; ++j){
    uint i = j % NBUCKET;
    acquire(&bcache.lock[i]);
    for(b = bcache.head[i].prev; b != &bcache.head[i]; b = b->prev){
      if(b->refcnt == 0) {
        
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        b->tick_num = ticks;

        b->prev->next = b->next;
        b->next->prev = b->prev;

        b->prev = &bcache.head[hashnum];
        b->next = bcache.head[hashnum].next;
        bcache.head[hashnum].next->prev = b;
        bcache.head[hashnum].next = b;

        release(&bcache.lock[i]);
        release(&bcache.globallock);
        release(&bcache.lock[hashnum]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.lock[i]);
  }
  release(&bcache.globallock);
  release(&bcache.lock[hashnum]);
  panic("bget: no buffers");
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

  releasesleep(&b->lock);

  // acquire(&bcache.globallock);
  uint hashnum = (b->dev * 8 + b->blockno) % NBUCKET;
  
  // printf("release dev: %d, blockno: %d, hashnum: %d\n", b->dev, b->blockno, hashnum);
  acquire(&bcache.lock[hashnum]);

  b->tick_num = ticks;
  // if (b->tick_num == ticks){
    b->refcnt--;
    if (b->refcnt == 0) {
      // no one is waiting for it.
      b->next->prev = b->prev;
      b->prev->next = b->next;
      b->next = bcache.head[hashnum].next;
      b->prev = &bcache.head[hashnum];
      bcache.head[hashnum].next->prev = b;
      bcache.head[hashnum].next = b;
    }
  // }
  release(&bcache.lock[hashnum]);
  // release(&bcache.globallock);
  
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock[b->dev*8+b->blockno]);
  b->refcnt++;
  release(&bcache.lock[b->dev*8+b->blockno]);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock[b->dev*8+b->blockno]);
  b->refcnt--;
  release(&bcache.lock[b->dev*8+b->blockno]);
}


