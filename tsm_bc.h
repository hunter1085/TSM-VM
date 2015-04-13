#ifndef _BC_GP_H_
#define _BC_GP_H_

#include "./common/fm_Bytes.h"

#define MAX_APDU_LEN    256

#define TAG_A0          0xA0
#define TAG_A1          0xA1

typedef enum {
	AG_BUSINESS = 0x00,
	AG_FUNCTION  = 0x01,
	AG_TOOL= 0xFF
}agent_type_t;

typedef struct {
	fmBytes *master;
	fmBytes *slave;
}master_slave_t;

#endif