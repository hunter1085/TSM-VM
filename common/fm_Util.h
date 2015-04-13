#ifndef _FM_UTIL_H_
#define _FM_UTIL_H_

#include "fm_Base.h"
#include "fm_Bytes.h"

char *fm_strstr(u8 *str,int len, u8 *sub_str,int sub_len);
int last_index_of(char *str,char delimiter);
void str_trim(char *pStr);
void trim_lr(char *ptr);
int sub_str(char *str,int start,int len,char *sub_str);
int str_2_hexstr(char *in,char *out);
int set_nonblocking(int fd);
void clear_buffer(int fd);
int init_thread_pool(int num,void *(*func)(void *));
int fmbytes_concat_with_len(u8 *des,fmBytes *src);
int fmbytes_concat_with_nlen(u8 *des,u8 len_size,fmBytes *src);
int get_BERLen(u32 len,u8 *berlen);
fmBool is_all_zero(u8 *buf,int len);
int int2byteBE(int v,u8 *buf);

#endif