#include "defs.h"
#include "riscv.h"
#include "spinlock.h"

struct freeblock_info // 4B
{
    uint16 offset; // 距离下一个空闲内存块的偏移量(Byte)
    uint16 size;
};

struct raw_page_info{
    struct raw_page_info* next;
    struct freeblock_info head;
};

struct{
    struct raw_page_info* first;
    struct raw_page_info* last;
    struct spinlock lock;
} raw_kmallocator;

void raw_kmalloc_init();
void* raw_kmalloc(uint32 sz, uint32 align);
void raw_kfree(void* pa);

static void dilatation()
{
    struct raw_page_info* newpage = (struct raw_page_info*)kalloc();
    if (newpage == 0)
        panic("raw_kmalloc: dilatation.");
    newpage->next = 0;
    newpage->head.offset = 0;
    newpage->head.size = PGSIZE - sizeof(struct raw_page_info) + sizeof(struct freeblock_info);

    if(raw_kmallocator.first == 0)
    {
        // 在初始化时
        raw_kmallocator.first = newpage;
        raw_kmallocator.last = newpage;
    }
    else
    {
        // 此时至少有一个内存页
        raw_kmallocator.last->next = newpage;
        raw_kmallocator.last = newpage;
    }
}

void raw_kmalloc_init()
{
    initlock(&raw_kmallocator.lock, "raw_kmalloc");
    acquire(&raw_kmallocator.lock);

    raw_kmallocator.first = 0;
    raw_kmallocator.last = 0;
    dilatation();

    release(&raw_kmallocator.lock);
}

void *raw_kmalloc(uint32 sz, uint32 align)
{
    if (sz >= PGSIZE)
        panic("raw_kmalloc: invalid size.");
    if (align & (align - 1))
        panic("raw_kmalloc: invalid alignment.");
    

    return NULL;
}

void raw_kfree(void *pa)
{
}
