#include "defs.h"
#include "riscv.h"
#include "slab.h"

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

struct kmem_cache g_slab_8;
struct kmem_cache g_slab_16;
struct kmem_cache g_slab_32;
struct kmem_cache g_slab_64;
struct kmem_cache g_slab_128;
struct kmem_cache g_slab_256;
struct kmem_cache g_slab_512;

struct kmem_cache g_slab_kmem_cache;

// TODO: 批量分配和批量释放, cpu专用缓存和共用缓存的区分

#define ALIGN_UP(sz, align) (((sz) + ((align) - 1)) & (~((align) - 1)))

struct snode* init_small_object_pool(uint16 unitsz, uint16 init_pgnum)
{
    // 隐式链接，把页的链接直接放在每个页面的首个对象的位置
    struct snode* firstpg = (struct snode*)(kalloc());
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
                volatile struct freeblock* fb = (struct freeblock*)(curpg + j * unitsz);
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
            volatile struct freeblock* fb = (struct freeblock*)(curpg + j * unitsz);
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

struct lnode* init_large_object_pool(uint16 unitsz, uint16 init_pgnum)
{
    // 显式链接，把页的链接放在g_slab_16中，依赖于g_slab_16的初始化
    struct lnode* result = (struct lnode*)kmem_cache_alloc(&g_slab_16);
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
            curnode->curpage = curpg;
            char* newpg = (char*)(kalloc());
            struct lnode* newnode = (struct lnode*)(kmem_cache_alloc(&g_slab_16));
            newnode->curpage = newpg;
            curnode->next = newnode;
            curnode = newnode;

            //最后一项和下一页的第一项链接
            struct freeblock* fb = (struct freeblock*)(curpg + iub * unitsz);
            fb->next = (struct freeblock*)(newpg);

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

void destroy_small_object_pool(struct kmem_cache* cache)
{
    struct snode* head = cache->head.shead;
    while(head != NULL)
    {
        struct snode* t = head->next;
        kfree(head);
        head = t;
    }
}

void destroy_large_object_pool(struct kmem_cache* cache)
{
    struct lnode* head = cache->head.lhead;
    while(head != NULL)
    {
        struct lnode* t = head->next;
        kfree(head->curpage);
        kmem_cache_free(&g_slab_16, head);
        head = t;
    }
}

void* alloc_small_object_pool(struct kmem_cache* cache)
{
    struct snode* head = cache->head.shead;
    struct freeblock* faptr = (struct freeblock*)((char*)head + cache->unitsz); // 第一个位置永远不会被分配，用来存储首个可用槽位
    struct freeblock* fa = faptr->next;
    if (fa == NULL)
    {
        // 把扩容出来的页放在第一页(可能需要优化，待测试)
        struct snode* newpg = init_small_object_pool(cache->unitsz, 1);
        newpg->next = head;
        cache->head.shead = newpg;
        uint16 unitsz = cache->unitsz;
        struct freeblock* last = (struct freeblock*)((char*)(newpg) + ((PGSIZE) / unitsz - 1) * unitsz);
        last->next = faptr;
        return alloc_small_object_pool(cache);
    }
    faptr->next = fa->next;
    release(&cache->lock);
    return fa;
}

void* alloc_large_object_pool(struct kmem_cache* cache)
{
    struct lnode* head = cache->head.lhead;
    struct freeblock* faptr = (struct freeblock*)(head->curpage); // 第一个位置永远不会被分配，用来存储首个可用槽位(待优化)
    struct freeblock* fa = faptr->next;
    if (fa == NULL)
    {
        // 扩容
        struct lnode* newnode = init_large_object_pool(cache->unitsz, 1);
        newnode->next = head;
        cache->head.lhead = newnode;
        uint16 unitsz = cache->unitsz;
        struct freeblock* last = (struct freeblock*)((char*)(newnode->curpage) + ((PGSIZE) / unitsz - 1) * unitsz);
        last->next = faptr;
        return alloc_large_object_pool(cache);
    }
    faptr->next = fa->next;
    release(&cache->lock);
    return fa;
}

inline void* pa_to_page(void* pa)
{
    return (void*)((uint64)pa & ~(PGSIZE - 1));
}

void free_small_object_pool(struct kmem_cache* cache, void* pa)
{
    struct snode* pg = (struct snode*)(pa_to_page(pa));
    struct snode* head = cache->head.shead;
    
    // 检查它是不是这个slab池里的
    struct snode* workptr = head;
    while(workptr != NULL && workptr != pg)
    {
        workptr = workptr->next;
    }
    if (workptr == NULL)
        panic("kmem_cache_free: invalid pa.");
    uint16 unitsz = cache->unitsz;
    if (((uint64)pa - (uint64)pg) % unitsz != 0)
        panic("kmem_cache_free: invalid alignment.");

    struct freeblock* faptr = (struct freeblock*)((char*)head + sizeof(struct snode)); // 第一个位置永远不会被分配，用来存储首个可用槽位
    ((struct freeblock*)(pa))->next = faptr->next;
    faptr->next = (struct freeblock*)(pa);
}

void free_large_object_pool(struct kmem_cache* cache, void* pa)
{
    char* pg = (char*)(pa_to_page(pa));
    struct lnode* head = cache->head.lhead;

    struct lnode* workptr = head;
    while(workptr != NULL && workptr->curpage != pg)
    {
        workptr = workptr->next;
    }
    if (workptr == NULL)
        panic("kmem_cache_free: invalid pa.");
    uint16 unitsz = cache->unitsz;
    if (((uint64)pa - (uint64)pg) % unitsz != 0)
        panic("kmem_cache_free: invalid alignment.");

    struct freeblock* faptr = (struct freeblock*)(head->curpage);
    ((struct freeblock*)(pa))->next = faptr->next;
    faptr->next = (struct freeblock*)(pa);
}

void slab_init()
{
    kmem_cache_create(&g_slab_8, "g_slab_8", 8, 8, 2);
    kmem_cache_create(&g_slab_16, "g_slab_16", 16, 16, 2);
    kmem_cache_create(&g_slab_32, "g_slab_32", 32, 32, 2);
    kmem_cache_create(&g_slab_64, "g_slab_64", 64, 64, 2);
    kmem_cache_create(&g_slab_128, "g_slab_128", 128, 128, 2);
    kmem_cache_create(&g_slab_256, "g_slab_256", 256, 256, 1);
    kmem_cache_create(&g_slab_512, "g_slab_512", 512, 512, 1);
    kmem_cache_create(&g_slab_kmem_cache, "g_slab_kmem_cache", sizeof(struct kmem_cache), 8, 1);
}

void kmem_cache_create(struct kmem_cache* cache, char* name, uint16 sz, uint16 align, uint16 init_pgnum)
{
    if(align == 0 || align & (align - 1))
        panic("kmem_cache_create: invalid alignment.");

    initlock(&(cache->lock), name);
    acquire(&cache->lock);

    cache->size = sz;
    cache->align = align;
    cache->unitsz = ALIGN_UP(sz, align);
    if (sz > 16)
    {
        cache->head.lhead = init_large_object_pool(cache->unitsz, init_pgnum);
    }
    else
    {
        cache->head.shead = init_small_object_pool(cache->unitsz, init_pgnum);
    }

    release(&cache->lock);
    printf("%s initialized.\n", name);
}

void kmem_cache_destroy(struct kmem_cache* cache)
{
    acquire(&cache->lock);
    if (cache->size > 16)
    {
        destroy_large_object_pool(cache);
    }
    else
    {
        destroy_small_object_pool(cache);
    }
    release(&cache->lock);
}

void *kmem_cache_alloc(struct kmem_cache * cache)
{
    acquire(&cache->lock);
    if(cache->size > 16)
    {
        return alloc_large_object_pool(cache);
    }
    else
    {
        return alloc_small_object_pool(cache);
    }
}

void kmem_cache_free(struct kmem_cache* cache, void* pa)
{
    acquire(&cache->lock);
    if(cache->size > 16)
    {
        free_large_object_pool(cache, pa);
    }
    else
    {
        free_small_object_pool(cache, pa);
    }
}