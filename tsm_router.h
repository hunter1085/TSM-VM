#ifndef _TSM_ROUTER_H_
#define _TSM_ROUTER_H_

#include <sys/epoll.h>

#include "./common/fm_List.h"
#include "./common/fm_Base.h"
#include "./common/fm_Bytes.h"
#include "./common/fm_Monitor.h"
#include "./common/fm_skbuff.h"
#include "tsm_cfg.h"

#define MSG_HEAD_LEN                12

#define PKG_REASON_NORMAL           0x00
#define PKG_REASON_BOOTSTRAP        0x01
#define PKG_REASON_TO               0x02

#define TAG_SESSION     0x10
#define TAG_DLL         0x11
#define TAG_SCRIPT      0x12
#define TAG_AG_NAME     0x13

#define INS_CREATE_CCB      0
#define INS_REG_AGENT       2
#define INS_UNREG_AGENT     4

#define ACK_REG_AGENT       3




typedef enum {
	MSG_CTRL = 0x01,
	MSG_DATA = 0x02,
}msg_type_t;



typedef struct{
	epoll_monitor_t monitor;
	int server;
	list_t agent_list;
	struct sk_buff_head rpkg_list;
	struct sk_buff_head wpkg_list;
	struct sk_buff_head ppkg_list;
}tsm_router_t;

typedef struct {
	struct list_head entry;
	u8 tag;
	u16 len;
	u8 *val;
}router_tlv_t;

typedef struct{
	int cnt;
	char **dls;
}dl_array_t;

PUBLIC int router_init(tsm_cfg_t *cfg,tsm_router_t **rt);
PUBLIC void *router_disaggregate_work(void* para);
PUBLIC void *router_aggregate_work(void* para);

#endif
