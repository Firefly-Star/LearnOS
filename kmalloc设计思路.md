void* kmalloc_page(): 返回内核空间中的一个4096B对齐的堆地址，并映射分配一页物理内存
void kfree_page(void*): 释放一页的堆内存

然后需要有一个raw_allocator，使用空闲分区链表和首次适应算法(暂定)，仿照umalloc中的算法，来管理分配小量且随机大小的小内存块。raw_allocator所管理的内存块使用kmalloc_page来进行分配。
raw_allocator需要对外提供
void* raw_kmalloc(int size): 分配一个size大小的堆内存

然后利用raw_allocator来构建slab和伙伴系统，slab是内核专用的，需要对外提供
void slab_init(): 初始化管理slab内存池的管理者，这个管理者是用raw_allocator来实现的
void slab_create(int size): 创建一个专门用于管理size大小对象的内存池
void* slab_kmalloc(): 分配一个slab对象
void slab_kfree(void*): 释放一个slab对象

初始应该就需要有16B, 32B, 64B, 128B对象所对应的slab内存池

伙伴系统则既可以用于内核的大内存分配，也可以用于用户的大内存分配