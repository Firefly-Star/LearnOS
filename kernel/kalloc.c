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

// 128MB的总物理内存，一页有4096B，也就是需要15位的位图来说明整体块的使用情况，刚好是一页的内存
// 可以做的优化：再分配一页来说明1阶及以上的块的使用情况，能提高buddy_free和buddy_alloc的效率(从o(n)到o(1))

#define MAX_ORDER (11)

void* pa_start;
uint64 pnum_max;

struct {
    void* bitmap_page;
    void* freelist_page;
    void* start;
    struct spinlock lock;
} buddy;

void* pnum_to_pa(uint64 pnum)
{
    if (pnum >= pnum_max)
    {
        panic("pnum_to_pa");
        return 0;
    }
    return (void*)((uint64)buddy.start + pnum * PGSIZE);
}

uint64 pa_to_pnum(void* pa)
{
    if ((uint64)pa >= PHYSTOP)
    {
        panic("pa_to_pnum");
        return ~0;
    }
    return ((uint64)(pa) - (uint64)buddy.start) / PGSIZE;
}

// 初始化buddy_链表
static void buddy_init_freelist()
{
    for (int i = 0;i < MAX_ORDER; ++i)
    {
        if (pnum_max & (1 << i))
        {
            uint64 pnum = pnum_max & (~((1 << (i + 1)) - 1));
            *((void**)((uint64)(buddy.freelist_page) + i * sizeof(void*))) = pnum_to_pa(pnum);
            *(void**)(pnum_to_pa(pnum)) = 0; // 0表示已经到链表的末尾了
        }
        else
        {
            *((void**)((uint64)(buddy.freelist_page) + i * sizeof(void*))) = 0;
        }
    }
    void* work_ptr = buddy.freelist_page + MAX_ORDER * sizeof(void*);
    for (uint64 pnum = 0;;)
    {
        void* next_page = pnum_to_pa(pnum);
        *((void**)(work_ptr)) = next_page;
        work_ptr = next_page;

        pnum += 1 << MAX_ORDER;
        if (pnum >= PHYSTOP)
        {
            *((void**)(work_ptr)) = 0;
            break;
        }
    }
}

void buddy_init(){
    initlock(&buddy.lock, "buddy");
    acquire(&buddy.lock);

    pa_start = (char*)PGROUNDUP((uint64)end);
    buddy.bitmap_page = pa_start; // 第一块内存分配给位图
    buddy.freelist_page = pa_start + PGSIZE; // 第二块分配给链表，这里的内存有一大半都是被浪费的，未来可以在里面加东西
    buddy.start = pa_start + 2 * PGSIZE;
    memset(buddy.bitmap_page, 0, PGSIZE); // 将位图初始化为0，即全部为未分配
    pnum_max = (PHYSTOP - (uint64)buddy.start) / PGSIZE;
    // 初始化链表
    buddy_init_freelist();

    release(&buddy.lock);
}

void* buddy_getfirst(uint32 order) // 构式一样的取名
{
    return *((void**)((uint64)buddy.freelist_page + order * sizeof(void*)));
}

void buddy_insertfirst(void* newfirst, uint32 order)
{
    *((void**)(newfirst)) = buddy_getfirst(order);
    *((void**)((uint64)buddy.freelist_page + order * sizeof(void*))) = newfirst;
}

void* buddy_erasefirst(uint32 order)
{
    void* result = buddy_getfirst(order);
    if (result == 0)
        panic("buddy_erasefirst.");
    *((void**)((uint64)buddy.freelist_page + order * sizeof(void*))) = *((void**)result);
    return result;
}

void buddy_setbitmap(uint64 pnum, uint32 order, uint32 flag)
{
    if (pnum & ((1 << order) - 1))
        panic("buddy_setbitmap");
    
    char* t = (char*)(buddy.bitmap_page) + (pnum / 8);
    if (order < 3)
    {
        char mask = ((1 << (1 << order)) - 1) << (pnum % 8);
        if (flag)
        {
            (*t) |= mask;
        }
        else
        {
            (*t) &= ~mask;
        }
    }
    else
    {
        if (flag)
        {
            memset(t, 255, 1 << (order - 3));
        }
        else
        {
            memset(t, 0, 1 << (order - 3));
        }
    }
}

uint32 buddy_getbitmap(uint64 pnum, uint32 order)
{
    if (pnum & ((1 << order) - 1))
        panic("buddy_getbitmap");
    
    char* t = (char*)(buddy.bitmap_page) + (pnum / 8);
    if (order < 3)
    {
        char mask = ((1 << (1 << order)) - 1) << (pnum % 8);
        if ((*t) & mask)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        for (int i = 0;i < 1 << (order - 3); ++i)
        {
            if (*(t + i))
            {
                return 1;
            }
        }
        return 0;
    }
}

void* buddy_alloc(uint32 order)
{
    if (order > MAX_ORDER)
        panic("buddy_alloc: order out of bound.");
    int free_order = order;
    while(free_order <= MAX_ORDER && (buddy_getfirst(free_order) == 0))
    {
        ++free_order;
    }

    if (free_order > MAX_ORDER)
        panic("buddy_alloc: bad alloc.");
    
    // 将free_order给逐步拆分成order阶的内存块
    void* mem_block = buddy_erasefirst(free_order);
    while(free_order > order)
    {
        --free_order;
        buddy_insertfirst((void*)((uint64)mem_block + (1 << free_order) * PGSIZE), free_order); // 把地址相对较高的给存入链表中
    }

    // 设置位图
    uint64 pnum = pa_to_pnum(mem_block);
    buddy_setbitmap(pnum, order, 1);
    return mem_block;
}

void buddy_free(void* block_start, uint32 order)
{
    uint64 pnum = pa_to_pnum(block_start);
    if (pnum & ((1 << order) - 1))
        panic("buddy_free");
    
    return;
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
