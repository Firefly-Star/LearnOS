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
    struct run* prev;
    struct run* next;
};

// 128MB的总物理内存，一页有4096B，也就是需要15位的位图来说明整体块的使用情况，刚好是一页的内存
// 可以做的优化：链表变为双向链表，可以让buddy的释放复杂度变为近乎o(1)

#define MAX_ORDER (11)

void* pa_start;
uint64 pnum_max;

struct {
    char bitmap_page[2 * PGSIZE];
    struct run freelist_page[MAX_ORDER + 1]; // 可以理解成一个地址数组，freelist_page[i]中存放的是i阶内存块的链表首地址
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
    for (int i = 0;i <= MAX_ORDER; ++i)
    {
        buddy.freelist_page[i].next = NULL;
        buddy.freelist_page[i].prev = NULL;
    }

    for (int i = 0;i < MAX_ORDER; ++i)
    {
        if (pnum_max & (1 << i))
        {
            uint64 pnum = pnum_max & (~((1 << (i + 1)) - 1));
            buddy_insertfirst(pnum_to_pa(pnum), i);
        }
    }
    for (uint64 pnum = 0; pnum + (1 << MAX_ORDER) <= pnum_max; pnum += (1 << MAX_ORDER))
    {
        buddy_insertfirst(pnum_to_pa(pnum), MAX_ORDER);
    }
}

void buddy_init_bitmap()
{
    for (int i = 0; i < (2 * PGSIZE); i ++)
    {
        *(uint8*)(buddy.bitmap_page + i) = 255;
    }
}

void buddy_init(){
    initlock(&buddy.lock, "buddy");
    acquire(&buddy.lock);

    pa_start = (char*)PGROUNDUP((uint64)end);
    buddy.start = pa_start;
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
    struct run* a = (struct run*)newfirst;
    a->prev = buddy.freelist_page + order;
    a->next = buddy.freelist_page[order].next;
    if (buddy.freelist_page[order].next != NULL)
    {
        buddy.freelist_page[order].next->prev = a;
    }
    buddy.freelist_page[order].next = a;
    buddy_setbitmap(pa_to_pnum(newfirst), order, 0);
}

void* buddy_erasefirst(uint32 order)
{
    struct run* next = buddy.freelist_page[order].next;
    if (next == 0)
        panic("buddy_erasefirst.");
    buddy.freelist_page[order].next = next->next;
    if (next->next != NULL)
    {
        buddy.freelist_page[order].next->prev = buddy.freelist_page + order;
    }
    buddy_setbitmap(pa_to_pnum(next), order, 1);
    return (void*)next;
}

void buddy_erase(void* ptr, uint32 order)
{
    struct run* current = (struct run*)ptr;
    if (current->prev == 0)
        panic("buddy_erase");
    current->prev->next = current->next;    
    if (current->next != NULL)
    {
        current->next->prev = current->prev;
    }
    buddy_setbitmap(pa_to_pnum(ptr), order, 1);
}

// 0000 - 0fff: 0阶
// 1000 - 17ff: 1阶
// 1800 - 1bff: 2阶
// 1c00 - 1dff: 3阶
// 1e00 - 1eff: 4阶
// 1f00 - 1f7f: 5阶
// 1f80 - 1fbf: 6阶
// 1fc0 - 1fdf: 7阶
// 1fe0 - 1fef: 8阶
// 1ff0 - 1ff7: 9阶
// 1ff8 - 1ffb: 10阶
// 1ffc - 1ffd: 11阶
// 1ffe       : 12阶

uint16 buddy_bitmap_offset[] = {
[0]     0x0000,
[1]     0x1000,
[2]     0x1800,
[3]     0x1c00,
[4]     0x1e00,
[5]     0x1f00,
[6]     0x1f80,
[7]     0x1fc0,
[8]     0x1fe0,
[9]     0x1ff0,
[10]    0x1ff8,
[11]    0x1ffc,
[12]    0x1ffe
};

void buddy_setbitmap(uint64 pnum, uint32 order, uint32 flag)
{
    if (pnum & ((1 << order) - 1))
        panic("buddy_setbitmap");
    
    char* start = (char*)buddy.bitmap_page + buddy_bitmap_offset[order];
    uint64 offset = pnum >> order;
    char mask = 1 << (offset % 8);
    if (flag)
    {
        start[offset / 8] |= mask;
    }
    else
    {
        start[offset / 8] &= (~mask);
    }
}

uint32 buddy_getbitmap(uint64 pnum, uint32 order)
{
    if (pnum & ((1 << order) - 1))
        panic("buddy_getbitmap");
    
    char* start = (char*)buddy.bitmap_page + buddy_bitmap_offset[order];
    uint64 offset = pnum >> order;
    char mask = 1 << (offset % 8);
    return start[offset / 8] & mask;
}

// 需要先加锁!!!
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
    if (free_order > order)
    {
        while(free_order > order)
        {
            --free_order;
            void* upper_block = (void*)((uint64)mem_block + (1 << free_order) * PGSIZE);
            buddy_insertfirst(upper_block, free_order); // 把地址相对较高的给存入链表中

        }
    }
    return mem_block;
}

// 需要先加锁!!!
void buddy_free(void* block_start, uint32 order)
{
    uint64 pnum = pa_to_pnum(block_start);
    if (pnum & ((1 << order) - 1))
    {
        printf("%lu\n", pnum);
        panic("buddy_free");
    }

    uint32 freeorder = order;
    
    while(freeorder < MAX_ORDER)
    {
        uint64 buddy_pnum = pnum ^ (1 << freeorder);
        if (buddy_pnum < pnum_max && !buddy_getbitmap(buddy_pnum, freeorder))
        {
            
            // 兄弟块空闲
            // 从freeorder阶的链表中，找到兄弟页表，移除它
            // 计算得到新的pnum
            struct run* buddy_ptr = pnum_to_pa(buddy_pnum);
            
            buddy_erase(buddy_ptr, freeorder);
            pnum &= ~(1 << freeorder);
            ++freeorder;
        }
        else
        {
            // 兄弟块不空闲
            // 将pnum对应的内存块放到freeorder的链表中
            buddy_insertfirst(pnum_to_pa(pnum), freeorder);
            break;
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

    memset(result, 0, PGSIZE);

    return result;
}

void*
kbuddy_alloc(uint32 order)
{
    void* result;

    acquire(&buddy.lock);
    result = buddy_alloc(order);
    release(&buddy.lock);

    return result;
}

void
kbuddy_free(void *pa, uint32 order)
{
    acquire(&buddy.lock);
    buddy_free(pa, order);
    release(&buddy.lock);
}