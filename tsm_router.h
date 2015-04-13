#ifndef _TSM_ROUTER_H_
#define _TSM_ROUTER_H_

#include <sys/epoll.h>

#include "./common/fm_List.h"
#include "./common/fm_Base.h"
#include "./common/fm_Bytes.h"
#include "./common/fm_Monitor.h"
#include "tsm_cfg.h"


#define PKG_REASON_NORMAL           0x00
#define PKG_REASON_BOOTSTRAP        0x01
#define PKG_REASON_TO               0x02


typedef enum {
	MSG_CTRL = 0x00,
	MSG_DATA = 0x80,
}msg_type_t;

typedef struct{
	epoll_monitor_t monitor;
	list_t agent_list;
	list_t rpkg_list;
	list_t wpkg_list;
	list_t ppkg_list;
}tsm_router_t;

typedef struct{
	int cnt;
	char **dls;
}dl_array_t;

PUBLIC int router_init(tsm_cfg_t *cfg,tsm_router_t **rt);
PUBLIC void *router_disaggregate_work(void* para);
PUBLIC void *router_aggregate_work(void* para);

#endif