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

#define ALIGN_UP(sz, align) ((sz + (align - 1)) & (~(align - 1)))

struct snode* init_small_object_pool(uint16 sz, uint16 align, uint16 init_pgnum)
{
    // 隐式链接，把页的链接直接放在每个页面的首个对象的位置
    struct snode* firstpg = (struct snode*)(kalloc());
    uint16 unitsz = ALIGN_UP(sz, align);
    uint16 eub = init_pgnum - 1;
    uint16 iub = (PGSIZE) / unitsz - 1;
    { // 初始化
        char* curpg = (char*)firstpg;
        // 前(init_pnum - 1)页
        for (int i = 0;i < eub; ++i)
        {
            // 前(PGSIZE) / unitsz - 1项
            for (int j = 1;j < iub; ++j)
            {
                struct freeblock* fb = (struct freeblock*)(curpg + j * unitsz);
                fb->next = (struct freeblock*)(curpg + (j + 1) * unitsz);
            }
            // 链接snode
            char* newpg = (char*)(kalloc());
            struct snode* t = (struct snode*)(curpg);
            t->next = (struct snode*)(newpg);

            // 最后一项和下一页的第一项链接
            struct freeblock* fb = (struct freeblock*)(curpg + iub * unitsz);
            fb->next = (struct freeblock*)(newpg + unitsz);

            curpg = newpg;
        }   
        // 第init_pnum页
        // 前(PGSIZE) / unitsz - 1项
        for (int j = 1;j < iub; ++j)
        {
            struct freeblock* fb = (struct freeblock*)(curpg + j * unitsz);
            fb->next = (struct freeblock*)(curpg + (j + 1) * unitsz);
        }
        // snode->next置空
        struct snode* t = (struct snode*)(curpg);
        t->next = NULL;

        // 最后一项的下一项置空C
        struct freeblock* fb = (struct freeblock*)(curpg + iub * unitsz);
        fb->next = NULL;
    }
    return firstpg;
}

struct lnode* init_large_object_pool(uint16 sz, uint16 align, uint16 init_pgnum)
{
    // 显式链接，把页的链接放在g_slab_16中，依赖于g_slab_16的初始化
    struct lnode* result = (struct lnode*)kmem_cache_alloc(&g_slab_16);
    uint16 unitsz = ALIGN_UP(sz, align);
    uint16 eub = init_pgnum - 1;
    uint16 iub = (PGSIZE) / unitsz - 1;
    { // 初始化
        char* firstpg = (char*)(kalloc());
        result->curpage = firstpg;
        char* curpg = firstpg;
        struct lnode* curnode = result;
        // 前init_pgnum - 1页
        for (int i = 0;i < eub; ++i)
        {
            // 前(PGSIZE) / unitsz - 1项
            for (int j = 0;j < iub; ++j)
            {
                struct freeblock* fb = (struct freeblock*)(curpg + j * unitsz);
                fb->next = (struct freeblock*)(curpg + (j + 1) * unitsz);
            }
            // 链接lnode
            char* newpg = (char*)(kalloc());
            curnode->next = (struct lnode*)(newpg);

            //最后一项和下一页的第一项链接
            struct freeblock* fb = (struct freeblock*)(curpg + iub * unitsz);
            fb->next = (struct freeblock*)(newpg);

            curnode = (struct lnode*)(kmem_cache_alloc(&g_slab_16));
            curpg = newpg;
        }
        // 最后一页
        // 前(PGSIZE) / unitsz - 1项
        for (int j = 0;j < iub; ++j)
        {
            struct freeblock* fb = (struct freeblock*)(curpg + j * unitsz);
            fb->next = (struct freeblock*)(curpg + (j + 1) * unitsz);
        }
        // lnode->next置空
        curnode->next = NULL;

        //最后一项和下一页的第一项链接
        struct freeblock* fb = (struct freeblock*)(curpg + iub * unitsz);
        fb->next = NULL;
    }

    return result;
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

void kmem_cache_create(struct kmem_cache* cache, char* name, uint16 sz, uint16 align, uint16 init_pgnum)
{
    if(align & (align - 1))
        panic("kmem_cache_create: invalid alignment.");

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

void kmem_cache_destroy(struct kmem_cache* cache)
{
    return;
}

void *kmem_cache_alloc(struct kmem_cache * cache)
{
    return NULL;
}

void kmem_cache_free(struct kmem_cache* cache, void* pa)
{
    return;
}
