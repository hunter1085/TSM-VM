#ifndef _TSM_CCB_H_
#define _TSM_CCB_H_

#include "./common/fm_Base.h"
#include "./common/fm_Bytes.h"
#include "./common/fm_List.h"
#include "./common/fm_hashtable.h"
#include "./common/fm_skbuff.h"
#include "tsm_scloader.h"


typedef int (*tsm_bytecode)(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg);

typedef struct {
    struct list_head entry;
	int session;
}agent_t;

typedef struct{
	struct list_head entry;
	int agent;
	int session;
	int msg_type;
	int reason;
	fmBytes *data;
}pkg_entry_t;

typedef struct{
	int cnt;
	tsm_bytecode *bc;
}byte_code_t;


typedef struct {
    struct list_head entry;

	int PC;
	fmBool write_back;
	fmBool bootstrap;
	int session;
	struct sk_buff_head cmdQ;
	struct sk_buff_head apduQ;
	byte_code_t *bc;
	script_entry_t *se;
	zc_hashtable_t *bre;  //point to business-relate runtime environment
}tsm_ccb_t;


#endif
