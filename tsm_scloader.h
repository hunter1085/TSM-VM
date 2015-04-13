#ifndef _TSM_SCLOADER_H_
#define _TSM_SCLOADER_H_

#include "./common/fm_List.h"
#include "./common/fm_Bytes.h"

#define MAX_PARM_NUM        20

typedef struct{
	struct list_head node;
	int addr;
    int code;
	fmBytes_array_t parameters;
}code_entry_t;

PUBLIC script_entry_t *scloader_load(char *path);
#endif
