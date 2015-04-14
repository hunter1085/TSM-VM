#include <stdlib.h>
#include "fm_Mem.h"
u32 mem_index = 0;
void *fm_malloc(size_t size)
{
#if 0
 fmsh_mem *p;
 
 p = (fmsh_mem *)malloc(sizeof(fmsh_mem)+size);
 p->size = size;
 p->index = mem_index++;
 return (void *)p->s;
#else
 return malloc(size);
#endif
}
void *fm_calloc(size_t n,size_t size)
{
#if 0
 fmsh_mem *p;
 
 p = (fmsh_mem *)calloc(sizeof(fmsh_mem)+size);
 p->size = size;
 p->index = mem_index++;
 return (void *)p->s;
#else
 return calloc(n,size);
#endif
}
void fm_free(void *mem_ptr)
{
#if 0
 fmsh_mem *p;
 p = container_of(mem_ptr,fmsh_mem,s);
 free(p);
#else
 free(mem_ptr);
#endif
}
void *fm_realloc(void *mem_address, unsigned int newsize)
{
    return realloc(mem_address,newsize);
}
