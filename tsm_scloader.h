#ifndef _TSM_SCLOADER_H_
#define _TSM_SCLOADER_H_

#include "./common/fm_List.h"
#include "./common/fm_Bytes.h"


typedef struct{
	int addr;
    int code;
	fmBytes_array_t *parameters;
}code_entry_t;

typedef struct{
	char *name;
	int size;
	int code_num;
	code_entry_t **ces;
}script_entry_t;

PUBLIC script_entry_t *scloader_load(char *path);
#endif
