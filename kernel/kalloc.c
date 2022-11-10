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
  struct spinlock lock[NCPU];
  struct run *freelist[NCPU];
} kmem;

void
kinit()
{
  for (int i = 0; i < NCPU; ++i)
    initlock(&kmem.lock[i], "kmem_lock_list");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  push_off();
  int cpu_id = cpuid();
  pop_off();

  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock[cpu_id]);
  r->next = kmem.freelist[cpu_id];
  kmem.freelist[cpu_id] = r;
  release(&kmem.lock[cpu_id]);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  push_off();
  int cpu_id = cpuid();
  pop_off();

  struct run *r;

  acquire(&kmem.lock[cpu_id]);
  r = kmem.freelist[cpu_id];
  if (!r) {
    // steal memory
    int i;
    for (i = 0; i < NCPU; ++i){
      if (i != cpu_id){
      // printf("%d %d!!\n", i, cpu_id);
        acquire(&kmem.lock[i]);
        if (kmem.freelist[i]){
          kmem.freelist[cpu_id] = kmem.freelist[i];
          kmem.freelist[i] = kmem.freelist[i]->next;
          kmem.freelist[cpu_id]->next = 0;

          r = kmem.freelist[cpu_id];
          kmem.freelist[cpu_id] = r->next;
          release(&kmem.lock[i]);
          break;
        }
        release(&kmem.lock[i]);
      }
    }
  }
  else{
    kmem.freelist[cpu_id] = r->next;
  }
  release(&kmem.lock[cpu_id]);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
