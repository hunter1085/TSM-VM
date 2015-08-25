#include "fm_Bytes.h"
#include "fm_Log.h"

LOCAL void fmBytes_set_size(fmBytes *p,int size)
{
    p->size = size;
}
PUBLIC fmBytes *fmBytes_alloc(int size)
{
    fmBytes *p;
	u8 *buf;
	
    p = (fmBytes *)fm_calloc(1,sizeof(fmBytes));
    if(p == NULL) return NULL;
    buf = (u8 *)fm_calloc(size,sizeof(u8));
    if(buf == NULL){
		fm_free(p);
        return NULL;
    }
    fmBytes_set_buf(p,buf);
	fmBytes_set_size(p,size);
	fmBytes_set_length(p,0);
    return p;
}

PUBLIC void fmBytes_free(fmBytes *p)
{
    u8 *buf = fmBytes_get_buf(p);
    if(buf != NULL)
        fm_free(buf);

    if(p != NULL)
        fm_free(p);
}

PUBLIC fmBool fmBytes_is_zeros(fmBytes *p,int offset,int len)
{
    int i;
	u8 *buf;

    buf = fmBytes_get_buf(p); buf += offset;
	for(i = 0; i < len; i++){
		if(buf[i] != 0)
			return fm_false;
	}
	return fm_true;
}

PUBLIC int fmBytes_copy(fmBytes *des,int offset1,fmBytes *src,int offset2,int len)
{
    u8 *des_buf,*src_buf;
	if((src == NULL) || (des == NULL)) return -1;
	if((fmBytes_get_buf(src) == NULL) || (fmBytes_get_buf(des) == NULL)) return -1;
	if((fmBytes_get_size(des)-offset1) < len) return -1;
	if((fmBytes_get_length(src)-offset2) < len) return -1;

    des_buf = fmBytes_get_buf(des); des_buf += offset1;
	src_buf = fmBytes_get_buf(src); src_buf += offset2;
	memcpy(des_buf,src_buf,len);
	fmBytes_set_length(des,fmBytes_get_length(des)+len);
	return 0;
}

PUBLIC int fmBytes_copy_from(fmBytes *des,int offset1,u8 *src,int offset2,int len)
{
    u8 *des_buf;

	if((src == NULL) || (des == NULL)) return -1;
	if((fmBytes_get_buf(des) == NULL)) return -1;
	if((fmBytes_get_size(des)-offset1) < len) return -1;

	

	des_buf = fmBytes_get_buf(des); des_buf += offset1;
	memcpy(des_buf,src+offset2,len);
	fmBytes_set_length(des,fmBytes_get_length(des)+len);
	return 0;
}

PUBLIC fmBool fmBytes_compare(fmBytes *p1,int offset1,
	fmBytes *p2,int offset2,int len)
{
    u8 *buf1,*buf2;
    if((p1 == NULL) || (p2 == NULL)) return fm_false;
	if((p1->buf == NULL)||(p2->buf == NULL)) return fm_false;
	if((fmBytes_get_length(p1) < (offset1 + len)) 
		||(fmBytes_get_length(p2) < (offset2 + len))) 
		return fm_false;

	buf1 = fmBytes_get_buf(p1); buf1 += offset1;
	buf2 = fmBytes_get_buf(p2); buf2 += offset2;
	if(memcmp(buf1,buf2,len) == 0) return fm_true;

	return fm_false;
}

PUBLIC u8 *fmBytes_get_buf(fmBytes *p)
{
    if(p == NULL) return NULL;
    return p->buf;
}
PUBLIC void fmBytes_set_buf(fmBytes *p,u8 *buf)
{
    if(p == NULL) return;
    p->buf = buf;
}
PUBLIC int fmBytes_get_size(fmBytes *p)
{
    return p->size;
}


PUBLIC int fmBytes_get_length(fmBytes *p)
{
    return p->leng;
}

PUBLIC void fmBytes_set_length(fmBytes *p,int len)
{
    p->leng = len = len;
}

PUBLIC int fmBytes2intBE(fmBytes *p)
{
    int i,ret;
	u8 *buf = fmBytes_get_buf(p);
    if(fmBytes_get_length(p)>4) return 0xFFFFFFFF;
	for(i = 0; i < fmBytes_get_length(p); i++){
		ret <<= i*8;
		ret |= buf[i];
	}
	return ret;
}

PUBLIC fmBytes_array_t *fmBytes_array_alloc(int size)
{
    fmBytes_array_t *p;
	p = fm_calloc(1,sizeof(fmBytes_array_t));
	if(!p)return NULL;
	p->size = size;
	p->cnt  = 0;
	p->array= fm_calloc(size,sizeof(fmBytes *));
	if(!p->array){
		fm_free(p);
		return NULL;
	}
	return p;
}

PUBLIC void fmBytes_array_free(fmBytes_array_t *fba)
{
    int i;
    if(!fba) return;

	for(i = 0; i < fba->cnt; i++){
		fmBytes_free(fba->array[i]);
	}
	fm_free(fba);
}

PUBLIC fmBytes_array_t *fmBytes_array_resize(fmBytes_array_t *old_fba,int new_size)
{
    int i,old_size;
	fmBytes_array_t *new_fba;

    old_size = old_fba->size;
	if(new_size <= old_size)return old_fba;

	new_fba = fmBytes_array_alloc(new_size);
	if(!new_fba) return old_fba;

	new_fba->cnt = old_fba->cnt;
	for(i = 0; i < old_fba->cnt; i++){
		new_fba->array[i] = old_fba->array[i];
	}
	return new_fba;
}