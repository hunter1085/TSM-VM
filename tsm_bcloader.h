#ifndef _TSM_BCLOADER_H_
#define _TSM_BCLOADER_H_
#include "./common/fm_List.h"
#include "tsm_ccb.h"
typedef struct {
	struct list_head entry;
	char *name;
	int base;
	int cnt;
	tsm_bytecode *bc;
}bc_set_t;
PUBLIC bc_set_t *bcloader_load(char *path,list_t *bc_list);
#endif
