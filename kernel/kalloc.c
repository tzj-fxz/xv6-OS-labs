// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  struct spinlock refcntlock; // ref cnt lock
  uint32 *refcnt; // record ref cnt of each page
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&kmem.refcntlock, "kmem_refcnt");

  // alloc memory for refcnt array
  uint64 refcnt_pages_cnt = ((PHYSTOP - (uint64)end) >> 12) + 1;
  uint64 refcnt_pages = ((refcnt_pages_cnt * sizeof(uint32)) >> 12) + 1;

  kmem.refcnt = (uint32*)end;
  freerange(end + (refcnt_pages << 12), (void*)PHYSTOP);
}

inline int get_ref_cnt_index(void *pa)
{
  return ((char *)pa - (char *)PGROUNDUP((uint64)end)) >> 12;
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
  {
    // init refcnt
    kmem.refcnt[get_ref_cnt_index((void*)p)] = 1;
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

  // only free when refcnt = 0
  acquire(&kmem.lock);
  if (--kmem.refcnt[get_ref_cnt_index(pa)]){
    release(&kmem.lock);
    return;
  }
  release(&kmem.lock);

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
  {
    kmem.freelist = r->next;
    // init refcnt = 1
    kmem.refcnt[get_ref_cnt_index((void*)r)] = 1;
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
