#ifndef _BC_DATA_H_
#define _BC_DATA_H_

#include "./common/fm_Bytes.h"
#define BC_BASE 64

int data_set(void *CCB,fmBytes *rsp,fmBytes_array_t *arg);
int data_extract_rsp(void *CCB,fmBytes *rsp,fmBytes_array_t *arg);
int data_extract(void *CCB,fmBytes *rsp,fmBytes_array_t *arg);
int data_add_len(void *CCB,fmBytes *rsp,fmBytes_array_t *arg);
int data_concat(void *CCB,fmBytes *rsp,fmBytes_array_t *arg);
int data_reverse(void *CCB,fmBytes *rsp,fmBytes_array_t *arg);


#endif
