#ifndef _TSM_CFG_H_
#define _TSM_CFG_H_
#include "./common/fm_List.h"
#include "./common/fm_Base.h"

typedef struct {
    struct list_head entry;
    int agent_id;
	char *name;
	char *agent_w_fifo;
	char *agent_r_fifo;
}agent_info_t;

typedef struct {
	int bc_parm_max;
	int bc_class_num;
	int bc_class_room;

	list_t ag_list;
}tsm_cfg_t;

PUBLIC tsm_cfg_t *cfg_load(char *cfg_path);
#endif
