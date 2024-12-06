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


void* pnum_to_pa(uint64 pnum);
uint64 pa_to_pnum(void* pa);
void buddy_init_freelist();
void buddy_init_bitmap();
void buddy_init();
void* buddy_getfirst(uint32 order);
void buddy_insertfirst(void* newfirst, uint32 order);
void* buddy_erasefirst(uint32 order);
void buddy_setbitmap(uint64 pnum, uint32 order, uint32 flag);
uint32 buddy_getbitmap(uint64 pnum, uint32 order);
void* buddy_alloc(uint32 order);
void buddy_free(void* block_start, uint32 order);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

// 128MB的总物理内存，一页有4096B，也就是需要15位的位图来说明整体块的使用情况，刚好是一页的内存
// 可以做的优化：再分配一页来说明1阶及以上的块的使用情况，能提高buddy_free和buddy_alloc的效率(从o(n)到o(1))

#define MAX_ORDER (11)

void* pa_start;
uint64 pnum_max;

struct {
    char* bitmap_page;
    struct run* freelist_page; // 可以理解成一个地址数组，freelist_page[i]中存放的是i阶内存块的链表首地址
    char* start;
    struct spinlock lock;
} buddy;

void* pnum_to_pa(uint64 pnum)
{
    if (pnum >= pnum_max)
    {
        printf("pnum: %lu, ", pnum);
        panic("pnum_to_pa");
    }
    return (void*)(buddy.start + pnum * PGSIZE);
}

uint64 pa_to_pnum(void* pa)
{
    if ((uint64)pa >= PHYSTOP)
    {
        printf("pa: %lu, ", (uint64)(pa));
        panic("pa_to_pnum");
    }
    return ((char*)(pa) - buddy.start) / PGSIZE;
}

// 初始化buddy_链表
void buddy_init_freelist()
{
    for (int i = 0;i < MAX_ORDER; ++i)
    {
        if (pnum_max & (1 << i))
        {
            uint64 pnum = pnum_max & (~((1 << (i + 1)) - 1));
            struct run* a = pnum_to_pa(pnum);
            buddy.freelist_page[i].next = a;
            buddy_setbitmap(pnum, i, 0);
            a->next = 0; // 0表示已经到链表的末尾了
        }
        else
        {
            buddy.freelist_page[i].next = 0;
        }
    }
    struct run* work_ptr = buddy.freelist_page + MAX_ORDER;
    for (uint64 pnum = 0;;)
    {
        struct run* next_page = pnum_to_pa(pnum); // 下一页的地址
        work_ptr->next = next_page;
        work_ptr = next_page;
        buddy_setbitmap(pnum, MAX_ORDER, 0);

        pnum += 1 << MAX_ORDER;
        if (pnum >= pnum_max)
        {
            work_ptr->next = 0;
            break;
        }
    }
}

void buddy_init_bitmap()
{
    memset(buddy.bitmap_page, 255, 2 * PGSIZE);
}

void buddy_init(){
    initlock(&buddy.lock, "buddy");
    acquire(&buddy.lock);

    pa_start = (char*)PGROUNDUP((uint64)end);
    buddy.bitmap_page = pa_start; // 第一、二块内存分配给位图
    buddy.freelist_page = pa_start + 2 * PGSIZE; // 第三块分配给链表，这里的内存有一大半都是被浪费的，未来可以在里面加东西
    buddy.start = pa_start + 3 * PGSIZE;
    pnum_max = (PHYSTOP - (uint64)buddy.start) / PGSIZE;
    printf("pnum_max: %lu\n", pnum_max);

    buddy_init_bitmap();
    buddy_init_freelist();

    release(&buddy.lock);
}

void* buddy_getfirst(uint32 order) // 构式一样的取名
{
    return buddy.freelist_page[order].next;
}

void buddy_insertfirst(void* newfirst, uint32 order)
{
    struct run* a = newfirst;
    a->next = buddy.freelist_page[order].next;
    buddy.freelist_page[order].next = a;
}

void* buddy_erasefirst(uint32 order)
{
    struct run* next = buddy.freelist_page[order].next;
    if (next == 0)
        panic("buddy_erasefirst.");
    buddy.freelist_page[order].next = next->next;
    return (void*)next;
}
// 0000 - 3fff: 0阶
// 4000 - 5fff: 1阶
// 6000 - 6fff: 2阶
// 7000 - 77ff: 3阶
// 7800 - 7bff: 4阶
// 7c00 - 7dff: 5阶
// 7e00 - 7eff: 6阶
// 7f00 - 7f7f: 7阶
// 7f80 - 7fbf: 8阶
// 7fc0 - 7fdf: 9阶
// 7fe0 - 7fef: 10阶
// 7ff0 - 7ff7: 11阶

uint16 buddy_bitmap_offset[] = {
[0]     0x0000,
[1]     0x4000,
[2]     0x6000,
[3]     0x7000,
[4]     0x7000,
[5]     0x7800,
[6]     0x7c00,
[7]     0x7e00,
[8]     0x7f00,
[9]     0x7f80,
[10]    0x7fc0,
[11]    0x7fe0,
[12]    0x7ff0
};

void buddy_setbitmap(uint64 pnum, uint32 order, uint32 flag)
{
    if (pnum & ((1 << order) - 1))
        panic("buddy_setbitmap");
    
    char* start = (char*)buddy.bitmap_page + buddy_bitmap_offset[order];
    uint64 bit_offset = pnum >> order;
    char mask = 1 << (bit_offset % 8);
    if (flag)
    {
        start[bit_offset / 8] |= mask;
    }
    else
    {
        start[bit_offset / 8] &= (~mask);
    }
}

uint32 buddy_getbitmap(uint64 pnum, uint32 order)
{
    if (pnum & ((1 << order) - 1))
        panic("buddy_getbitmap");
    
    char* start = (char*)buddy.bitmap_page + buddy_bitmap_offset[order];
    uint64 bit_offset = pnum >> order;
    char mask = 1 << (bit_offset % 8);
    return start[bit_offset / 8] & mask;
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
    uint64 pnum = pa_to_pnum(mem_block);
    if (free_order > order)
    {
        buddy_setbitmap(pa_to_pnum(mem_block), free_order, 1);
        while(free_order > order)
        {
            // 设置位图
            buddy_setbitmap(pnum, free_order, 1);
            --free_order;
            void* upper_block = (void*)((uint64)mem_block + (1 << free_order) * PGSIZE);
            buddy_insertfirst(upper_block, free_order); // 把地址相对较高的给存入链表中
            buddy_setbitmap(pa_to_pnum(upper_block), free_order, 0);
        }
    }
    buddy_setbitmap(pnum, order, 1);
    return mem_block;
}

void buddy_free(void* block_start, uint32 order)
{
    uint64 pnum = pa_to_pnum(block_start);
    if (pnum & ((1 << order) - 1))
    {
        printf("%lu\n", pnum);
        panic("buddy_free");
    }

    buddy_setbitmap(pnum, order, 0);
    uint32 freeorder = order;
    
    while(freeorder < MAX_ORDER)
    {
        buddy_setbitmap(pnum, freeorder, 0);
        uint64 buddy_pnum = pnum ^ (1 << freeorder);
        if (buddy_pnum < pnum_max && buddy_getbitmap(buddy_pnum, freeorder))
        {
            // 将pnum对应的内存块放到freeorder的链表中
            buddy_insertfirst(pnum_to_pa(pnum), freeorder);
            break;
        }
        else
        {
            // 从freeorder阶的链表中，找到兄弟页表，移除它
            // 计算得到新的pnum
            struct run* buddy_ptr = pnum_to_pa(buddy_pnum);
            
            struct run* workptr = buddy.freelist_page + freeorder;
            while(workptr != 0 && workptr->next != buddy_ptr)
            {
                workptr = workptr->next;
            }
            workptr->next = workptr->next->next;
            pnum &= ~(1 << freeorder);
            ++freeorder;
        }
    }
    if (freeorder == MAX_ORDER)
    {
        buddy_insertfirst(pnum_to_pa(pnum), freeorder);
    }

    return;
}


extern int
printf(char *fmt, ...);

// #define XXX

void
kinit()
{
    buddy_init();
}

void
kfree(void *pa)
{
    acquire(&buddy.lock);
    buddy_free(pa, 0);
    release(&buddy.lock);
}

void *
kalloc(void)
{
    void* result;

    acquire(&buddy.lock);
    result = buddy_alloc(0);
    release(&buddy.lock);

    return result;
}
