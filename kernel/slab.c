#include "defs.h"

struct variant{
    void* next;
    int valid_flag;
};

/*
对象字节数    初始分配的页数  
16 字节		  2	
32 字节		  2	
64 字节		  2	
128 字节	  2	
256 字节	  2	
512 字节	  1	
1             1	
2	          1	
>=4KB		使用多个页组合大对象
*/

void slab_init_page()
{

}