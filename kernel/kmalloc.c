#include "defs.h"
#include "slab.h"
#include "riscv.h"

extern struct kmem_cache g_slab_8;
extern struct kmem_cache g_slab_16;
extern struct kmem_cache g_slab_32;
extern struct kmem_cache g_slab_64;
extern struct kmem_cache g_slab_128;
extern struct kmem_cache g_slab_256;
extern struct kmem_cache g_slab_512;
extern struct kmem_cache g_slab_1024;

void* kmalloc(uint64 sz)
{
    if (sz > 1024)
    {
        uint pgnum = ((sz + PGSIZE - 1) / PGSIZE);
        int order = 0;
        while((1U << order) < pgnum)
        {
            ++order;
        }
        return kbuddy_alloc(order);
    }
    else
    {
        if (sz > 512)
        {
            return kmem_cache_alloc(&g_slab_1024);
        }
        else if(sz > 256)
        {
            return kmem_cache_alloc(&g_slab_512);
        }
        else if(sz > 128)
        {
            return kmem_cache_alloc(&g_slab_256);
        }
        else if(sz > 64)
        {
            return kmem_cache_alloc(&g_slab_128);
        }
        else if(sz > 32)
        {
            return kmem_cache_alloc(&g_slab_64);
        }
        else if(sz > 16)
        {
            return kmem_cache_alloc(&g_slab_32);
        }
        else if(sz > 8)
        {
            return kmem_cache_alloc(&g_slab_16);
        }
        else
        {
            return kmem_cache_alloc(&g_slab_8);
        }
    }
}

void kmfree(void* ptr, uint64 sz)
{
    if (sz > 1024)
    {
        uint pgnum = ((sz + PGSIZE - 1) / PGSIZE);
        int order = 0;
        while((1U << order) < pgnum)
        {
            ++order;
        }
        kbuddy_free(ptr, order);
    }
    else
    {
        if (sz > 512)
        {
            kmem_cache_free(&g_slab_1024, ptr);
        }
        else if(sz > 256)
        {
            kmem_cache_free(&g_slab_512, ptr);
        }
        else if(sz > 128)
        {
            kmem_cache_free(&g_slab_256, ptr);
        }
        else if(sz > 64)
        {
            kmem_cache_free(&g_slab_128, ptr);
        }
        else if(sz > 32)
        {
            kmem_cache_free(&g_slab_64, ptr);
        }
        else if(sz > 16)
        {
            kmem_cache_free(&g_slab_32, ptr);
        }
        else if(sz > 8)
        {
            kmem_cache_free(&g_slab_16, ptr);
        }
        else
        {
            kmem_cache_free(&g_slab_8, ptr);
        }
    }
}