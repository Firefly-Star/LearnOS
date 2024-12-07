#include "defs.h"
#include "riscv.h"
#include "spinlock.h"

struct freeblock_info
{
    uint16 offset; // 距离下一个空闲内存块的偏移量(Byte)
    uint16 size;
};

struct raw_page_info{
    struct raw_page_info* next;
    uint16 head;
};

struct{
    struct raw_page_info* first;
    struct raw_page_info* last;
    struct spinlock lock;
} raw_kmallocator;

void raw_kmalloc_init();
void raw_kmalloc(uint32 sz);
void raw_free(void* pa);

static struct raw_page_info* dilatation()
{
    struct raw_page_info* newpage = (struct raw_page_info*)kalloc();
    newpage->next = 0;
    newpage->head = sizeof(struct raw_page_info);
    
}

void raw_kmalloc_init()
{
    initlock(&raw_kmallocator.lock, "raw_kmalloc");
    acquire(&raw_kmallocator.lock);

    release(&raw_kmallocator.lock);
}