#ifndef _BC_GP_H_
#define _BC_GP_H_

#include "./common/fm_Bytes.h"

#define BC_BASE 0

int gp_select(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg);
int gp_init_update(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg);
int gp_ext_auth(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg);
int gp_get_status(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg);
int gp_install_for_install_make_selectable(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg);
int gp_install_for_personalize(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg);
int gp_put_key(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg);
int gp_install_for_load_and_load(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg);
int gp_install_for_extradition(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg);
int gp_store_data(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg);
int gp_traverse(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg);
int gp_msg_macing(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg);
#endif
