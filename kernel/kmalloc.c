#include "types.h"
#include "stat.h"
#include "param.h"
#include "defs.h"

// 内存块头定义
typedef long Align;

union header {
  struct {
    union header *ptr;
    uint size;
  } s;
  Align x;
};

typedef union header Header;

static Header base;
static Header *freep;

// 内存释放
void kfree(void *ap)
{
  Header *bp, *p;
  bp = (Header*)ap - 1;
  for(p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
    if(p >= p->s.ptr && (bp > p || bp < p->s.ptr))
      break;

  if(bp + bp->s.size == p->s.ptr){
    bp->s.size += p->s.ptr->s.size;
    bp->s.ptr = p->s.ptr->s.ptr;
  } else
    bp->s.ptr = p->s.ptr;

  if(p + p->s.size == bp){
    p->s.size += bp->s.size;
    p->s.ptr = bp->s.ptr;
  } else
    p->s.ptr = bp;

  freep = p;
}

// 分配内存
void* kmalloc(uint nbytes)
{
  Header *p, *prevp;
  uint nunits;

  nunits = (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;
  
  if ((prevp = freep) == 0) {
    base.s.ptr = freep = prevp = &base;
    base.s.size = 0;
  }

  for (p = prevp->s.ptr; ; prevp = p, p = p->s.ptr) {
    if (p->s.size >= nunits) {
      if (p->s.size == nunits)
        prevp->s.ptr = p->s.ptr;
      else {
        p->s.size -= nunits;
        p += p->s.size;
        p->s.size = nunits;
      }
      freep = prevp;
      return (void*)(p + 1);
    }
    if (p == freep)
      if ((p = morecore(nunits)) == 0)
        return 0;
  }
}

// 请求更多内存
static Header* morecore(uint nu)
{
  char *p;
  Header *hp;

  if (nu < 4096)
    nu = 4096;
  p = (char*)kalloc();  // 使用内核分配函数替代 sbrk
  if (p == (char*)0)
    return 0;
  hp = (Header*)p;
  hp->s.size = nu;
  kfree((void*)(hp + 1));  // 将分配的内存块加入空闲链表
  return freep;
}
