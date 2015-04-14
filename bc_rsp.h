#ifndef _BC_RSP_H_
#define _BC_RSP_H_

#include "./common/fm_Bytes.h"

#define BC_BASE 32

int rsp_test_tail_jne(void *CCB,fmBytes *rsp,fmBytes_array_t *arg);
int rsp_test_tail_je(void *CCB,fmBytes *rsp,fmBytes_array_t *arg);
int rsp_test_head_jne(void *CCB,fmBytes *rsp,fmBytes_array_t *arg);
int rsp_test_head_je(void *CCB,fmBytes *rsp,fmBytes_array_t *arg);
int rsp_is_scp02_ok(void *CCB,fmBytes *rsp,fmBytes_array_t *arg);
int rsp_is_app_exit_je(void *CCB,fmBytes *rsp,fmBytes_array_t *arg);
int rsp_is_personalized_je(void *CCB,fmBytes *rsp,fmBytes_array_t *arg);
int rsp_is_card_active_je(void *CCB,fmBytes *rsp,fmBytes_array_t *arg);

#endif
