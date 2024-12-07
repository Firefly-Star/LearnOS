#include "defs.h"
#include "spinlock.h"

struct freeblock{
    struct freeblock* next;
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
1024 字节     1	
2048 字节	  1	
>=4KB		使用多个页组合大对象
*/

struct kmem_cache{
    char* name;
    uint16 size;
    struct freeblock head;
    struct spinlock lock;
};

struct kmem_cache* kmem_cache_create(const char* name, uint16 sz, uint16 init_pgnum);
struct kmem_cache* kmem_cache_destroy(const char* name, uint16 sz);
void* kmem_cache_alloc(struct kmem_cache *cachep);
void kmem_cache_free(struct kmem_cache *cachep, void *objp);
// TODO: 批量分配和批量释放, cpu专用缓存和共用缓存的区分

struct kmem_cache* kmem_cache_create(const char* name, uint16 sz, uint16 init_pgnum)
{
    
}