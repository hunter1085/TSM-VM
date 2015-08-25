#include "bc.h"
#include "./common/fm_hashtable.h"
#include "bc_data.h"
#include "tsm_vm.h"

/*suppose p points at the start of A1,the function returns the raw data(not include A0 len seq
and sw) at index=seq,if exist,otherwise,NULL*/
LOCAL fmBytes *_get_Tag_A0_at(u8 *p,u8 seq)
{
    u8 *point,*end,*buf;
	int len,num = 0;
    fmBytes *data;
	
	len = _get_Tag_Ax_len(p);
    point = _skip_Tag_Ax(p);
	end = point + len;
	
	while(point < end){
		if(*point == TAG_A0){
			len = _get_Tag_Ax_len(point);
			point = _skip_Tag_Ax(point);
			if(*point == seq){
				data = fmBytes_alloc(len-3);//not include seq and sw
				buf = fmBytes_get_buf(data);
				memcpy(buf,++point,len-3);
				fmBytes_set_length(data,len-3);
				return data;
			}
			point += len;
		}else{
		    break;
		}
	}
	return NULL;
	
}

int data_request(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg)
{
    return 0;
}

/*
assignment of data,format is as following:
data_set,data0:name0,data1:name1
*/
int data_set(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg)
{
    int i;
	zc_hashtable_t *bre = ccb_get_bre(CCB);
	
    if((arg->cnt%2) != 0) return -1;

	for(i = 0; i < arg->cnt; i += 2){
		int len = fmBytes_get_length(arg->array[i])/2;
		fmBytes *hex=fmBytes_alloc(len);
		hexstr_to_hex(arg->array[i],hex);
		zc_hashtable_put(bre,arg->array[i+1],hex);
	}
	return 0;
}
/*
extract data from rsp,format as following:
data_extract,frm:start:end:name,frm:start:end:name,frm:start:end:name
*/
int data_extract_rsp(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg)
{
    int i;
	zc_hashtable_t *bre = ccb_get_bre(CCB);

	if((arg->cnt%4) != 0) return -1;

	for(i = 0; i < arg->cnt; i += 4){
		int frm,start,end,name_len;
		char *name;
		fmBytes *raw,*data;
		frm   = fmBytes2intBE(arg->array[i+0]);
		start = fmBytes2intBE(arg->array[i+1]);
		end   = fmBytes2intBE(arg->array[i+2]);
		name_len = fmBytes_get_length(arg->array[i+3]);
		name  = (char *)fm_calloc(name_len+1,sizeof(char));
		name[name_len] = '\0';

        raw = rsp->array[frm];
		data = fmBytes_alloc(end-start+1);
		fmBytes_copy(data,0,raw,start,end-start+1);
		zc_hashtable_put(bre,name,data);
	}
	return 0;
}
/*
extract data from name,format as following:
data_extract,src0:start0:end0:dest0,src1:start1:end1:dest1
*/
int data_extract(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg)
{
    int i;
	zc_hashtable_t *bre = ccb_get_bre(CCB);

	if((arg->cnt%4) != 0) return -1;

	for(i = 0; i < arg->cnt; i += 4){
		int frm,start,end,name_len;
		char *name;
		fmBytes *raw,*data,*src_data;
		raw = data_get_by_name(CCB,arg->array[i+0]);
		start = fmBytes2intBE(arg->array[i+1]);
		end   = fmBytes2intBE(arg->array[i+2]);
		name_len = fmBytes_get_length(arg->array[i+3]);
		name  = (char *)fm_calloc(name_len+1,sizeof(char));
		name[name_len] = '\0';

		data = fmBytes_alloc(end-start+1);
		fmBytes_copy(data,0,raw,start,end-start+1);
		zc_hashtable_put(bre,name,data);
	}
	return 0;
}
/*
add len for data,format as following:
data_add_len,nameS0:nameD0,nameS1:nameD1
*/
int data_add_len(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg)
{
    //for test now
    return 5;
}
/*
concat datas together,format as following:
data_concat,sub0,sub1,subn,name
*/
int data_concat(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg)
{
    int i,len=0,offset = 0;
	fmBytes *des,*name;
	zc_hashtable_t *bre = ccb_get_bre(CCB);

	for(i = 0; i < arg->cnt-1; i++){
		fmBytes *sub;
		sub = data_get_by_name(CCB,arg->array[i]->buf);
		len += fmBytes_get_length(sub);
	}
	name = data_get_by_name(CCB,arg->array[i]->buf);
	
	des = fmBytes_alloc(len);
	for(i = 0; i < arg->cnt-1; i++){
		fmBytes *sub;
		sub = data_get_by_name(CCB,arg->array[i]->buf);
		len = fmBytes_get_length(sub);
		fmBytes_copy(des,offset,sub,0,len);
		offset += len;
	}
	zc_hashtable_put(bre,name->buf,des);
	return 0;
}
/*
reverse the data,format as following:
data_reverse,src0:des0,src1:des1
*/
int data_reverse(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg)
{
    int i,j;
	zc_hashtable_t *bre = ccb_get_bre(CCB);
	
    if((arg->cnt%2) != 0) return -1;

	for(i = 0; i < arg->cnt; i += 2){
		int len;
		u8 *str0,*str1;
		fmBytes *src,*des,*des_data;
		src = data_get_by_name(CCB,arg->array[i]->buf);
		des = arg->array[i+1];
		len = fmBytes_get_length(src);
		des_data = fmBytes_alloc(len);
		str0 = fmBytes_get_buf(src);
		str1 = fmBytes_get_buf(des_data);
		for(j = 0; j < len; j++){
			str1[j] = ~str0[j];
		}
		zc_hashtable_put(bre,des->buf,des_data);
	}
	return 0;
}