// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#define PNUM_TO_PA(x) (end + (x) * PGSIZE)
#define PA_TO_PNUM(x) ()


void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

void* pa_start;
uint64 pnum_max;

void pa_start_init(){
    pa_start = (char*)PGROUNDUP((uint64)pa_start);
    pnum_max = (PHYSTOP - (uint64)pa_start) / PGSIZE;
}

void* pnum_to_pa(uint64 pnum)
{
    if (pnum >= pnum_max)
    {
        panic("pnum_to_pa");
        return 0;
    }
    return (void*)((uint64)pa_start + pnum * PGSIZE);
}

uint64 pa_to_pnum(void* pa)
{
    if ((uint64)pa >= PHYSTOP)
    {
        panic("pa_to_pnum");
        return ~0;
    }
    return ((uint64)(pa) - (uint64)pa_start) / PGSIZE;
}

// 128MB的总物理内存，一页有4096B，也就是需要15位的位图来说明整体块的使用情况，刚好是一页的内存
// 可以做的优化：再分配一页来说明1阶及以上的块的使用情况，能提高buddy_free的效率

void* buddy_bitmap_page;
void* buddy_list_head;
struct spinlock buddy_lock;

void buddy_init()
{
    initlock(&buddy_lock, "buddy");
    acquire(&buddy_lock);
    buddy_bitmap_page = pa_start;
    memset(buddy_bitmap_page, 0, PGSIZE);
    // 分配第一块给位图，分配第二块作为不同阶的buddy的首指针。
    release(&buddy_lock);
}

void* buddy_alloc(uint32 order)
{

}

void* buddy_free(void* block_start, uint32 order)
{

}

void buddy_setbitmap(uint64 pnum, uint32 order, int flag)
{
    
}

extern int
printf(char *fmt, ...);

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

uint64 pgNumber;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
  pgNumber = (PHYSTOP - (uint64)end) / PGSIZE;
  printf("spare page number: %lu\n", pgNumber);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  ++pgNumber;
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
    --pgNumber;
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
