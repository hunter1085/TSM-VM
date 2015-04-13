#ifndef _FM_BYTES_H_
#define _FM_BYTES_H_
#include "fm_Base.h"
typedef struct _fmBytes{
    int size;
    int leng;
    u8  *buf;
}fmBytes;

typedef struct
{
    int size;
	fmBytes **array;
}fmBytes_array_t;

typedef struct
{
    struct list_head entry;
	fmBytes *data;
}fmBytes_list_entry_t;

PUBLIC fmBytes *fmBytes_alloc(int size);
PUBLIC void fmBytes_free(fmBytes *p);
PUBLIC fmBool fmBytes_is_zeros(fmBytes *p,int offset,int len);
PUBLIC int fmBytes_copy(fmBytes *des,int offset1,fmBytes *src,int offset2,int len);
PUBLIC fmBool fmBytes_compare_at(fmBytes *p1,int offset1,
	fmBytes *p2,int offset2,int len);
PUBLIC u8 *fmBytes_get_buf(fmBytes *p);
PUBLIC void fmBytes_set_buf(fmBytes *p,u8 *buf);
PUBLIC int fmBytes_get_size(fmBytes *p);
PUBLIC int fmBytes_get_length(fmBytes *p);
PUBLIC void fmBytes_set_length(fmBytes *p,int len);
PUBLIC int fmBytes2intBE(fmBytes *p);
PUBLIC fmBytes_array_t *fmBytes_array_alloc(int size);
#endif