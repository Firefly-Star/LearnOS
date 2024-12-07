#include "defs.h"
#include "spinlock.h"
#include "riscv.h"

struct freeblock{
    struct freeblock* next;
};

struct snode{
    struct snode* next;
};

struct lnode{
    char* curpage;
    struct lnode* next;
};

union pagelink{
    struct snode* shead;
    struct lnode* lhead;
};

/*
对象字节数    初始分配的页数  
8 字节        2 
16 字节		  2	
32 字节		  2	
64 字节		  2	
128 字节	  2	
256 字节	  2	
512 字节	  1	
>=4KB		使用多个页组合大对象
*/

struct kmem_cache{
    char* name;
    uint16 size;
    uint16 align;
    union pagelink head;
    struct spinlock lock;
};

struct kmem_cache g_slab_8;
struct kmem_cache g_slab_16;
struct kmem_cache g_slab_32;
struct kmem_cache g_slab_64;
struct kmem_cache g_slab_128;
struct kmem_cache g_slab_256;
struct kmem_cache g_slab_512;

// TODO: 批量分配和批量释放, cpu专用缓存和共用缓存的区分

struct snode* init_small_object_pool(uint16 sz, uint16 align, uint16 init_pgnum)
{
    // 隐式链接，把页的链接直接放在每个页面的首个对象的位置
}

struct lnode* init_large_object_pool(uint16 sz, uint16 align, uint16 init_pgnum)
{
    // 显式链接，把页的链接放在g_slab_16中，依赖于g_slab_16的初始化
}

void slab_init()
{
    kmem_cache_create(&g_slab_8, "g_slab_8", 8, 8, 2);
    kmem_cache_create(&g_slab_16, "g_slab_16", 16, 16, 2);
    kmem_cache_create(&g_slab_32, "g_slab_32", 32, 32, 2);
    kmem_cache_create(&g_slab_64, "g_slab_64", 64, 64, 2);
    kmem_cache_create(&g_slab_128, "g_slab_128", 128, 128, 2);
    kmem_cache_create(&g_slab_256, "g_slab_256", 256, 246, 1);
    kmem_cache_create(&g_slab_512, "g_slab_512", 512, 512, 1);
}

void kmem_cache_create(struct kmem_cache* cache, const char* name, uint16 sz, uint16 align, uint16 init_pgnum)
{
    initlock(&(cache->lock), name);
    acquire(&cache->lock);

    cache->size = sz;
    cache->align = align;
    if (sz > 16)
    {
        cache->head.lhead = init_large_object_pool(sz, align, init_pgnum);
    }
    else
    {
        cache->head.shead = init_small_object_pool(sz, align, init_pgnum);
    }

    release(&cache->lock);
}

void kmem_cache_destroy(struct kmem_cache* cache, uint16 sz, uint16 align)
{
    return;
}