#ifndef _BC_H_
#define _BC_H_

#define MAX_APDU_LEN        300
#define TAG_A1_HEAD_ROOM    4   //A1-len,len may cost 3Bytes
#define TAG_A0_HEAD_ROOM    3   //A0-len-seq



typedef enum {
	AG_BUSINESS = 0x00,
	AG_FUNCTION  = 0x01,
	AG_TOOL= 0xFF
}agent_type_t;

#endif