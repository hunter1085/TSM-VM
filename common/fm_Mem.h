#ifndef _FM_MEM_H_
#define _FM_MEM_H_
#include "fm_Base.h"
typedef struct {
 u32 size;
 u32 index;
 char s[0];
}fmsh_mem;
void *fm_malloc(size_t size);
void *fm_calloc(size_t n,size_t size);
void fm_free(void *mem_ptr);
#endif
