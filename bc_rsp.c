#include "./common/fm_hashtable.h"
#include "tsm_Err.h"
#include "bc.h"
#include "bc_rsp.h"


LOCAL fmBool _rsp_test_tail(fmBytes_array_t *rsp,fmBytes *exp_buf)
{
    int i,offset,len;
	fmBytes *frag;

	for(i = 0; i < rsp->cnt; i++){
		frag = rsp->array[i];
		len = fmBytes_get_length(exp_buf);
		offset = fmBytes_get_length(frag)-len;
		if(fmBytes_compare(frag,offset,exp_buf,0,len)==fm_false)return fm_false;
	}
	return fm_true;
}
LOCAL int _rsp_test_head(fmBytes_array_t *rsp,fmBytes *exp_buf)
{
    int i,len;
	fmBytes *frag;

	for(i = 0; i < rsp->cnt; i++){
		frag = rsp->array[i];
		len = fmBytes_get_length(exp_buf);
		if(fmBytes_compare(frag,0,exp_buf,0,len)==fm_false)return fm_false;
	}
	return fm_true;
}

/****************************************************************************/


int rsp_test_tail_jne(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg)
{
    int addr;
    fmBytes *exp_buf,*addr_buf;

    if(arg->size != 2) return FM_GEN_ERR_INVALID_PARM;
	
    exp_buf = arg->array[0];
	addr_buf = arg->array[1];
	addr = fmBytes2intBE(addr_buf);    
    if(!_rsp_test_tail(rsp,exp_buf)){
		ccb_set_PC(CCB,addr);
    }
	return 0;
}

int rsp_test_tail_je(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg)
{
    int addr;
    fmBytes *exp_buf,*addr_buf;

    if(arg->size != 2) return FM_GEN_ERR_INVALID_PARM;
	
    exp_buf = arg->array[0];
	addr_buf = arg->array[1];
	addr = fmBytes2intBE(addr_buf);    
    if(_rsp_test_tail(rsp,exp_buf)){
		ccb_set_PC(CCB,addr);
    }
	return 0;
}

int rsp_test_head_jne(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg)
{
    int addr;
    fmBytes *exp_buf,*addr_buf;

    if(arg->size != 2) return FM_GEN_ERR_INVALID_PARM;
	
    exp_buf = arg->array[0];
	addr_buf = arg->array[1];
	addr = fmBytes2intBE(addr_buf);    
    if(_rsp_test_head(rsp,exp_buf) != 0){
		ccb_set_PC(CCB,addr);
    }
	return 0;
}

int rsp_test_head_je(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg)
{
    int addr;
    fmBytes *exp_buf,*addr_buf;

    if(arg->size != 2) return FM_GEN_ERR_INVALID_PARM;
	
    exp_buf = arg->array[0];
	addr_buf = arg->array[1];
	addr = fmBytes2intBE(addr_buf);    
    if(_rsp_test_head(rsp,exp_buf) == 0){
		ccb_set_PC(CCB,addr);
    }
	return 0;
}

int rsp_is_scp02_ok(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg)
{
    int addr;
	fmBytes *addr_buf = arg->array[0];
    fmBytes *card_crypt = data_get_by_name(CCB,"card cryptogram");
	fmBytes *calc_crypt = data_get_by_name(CCB,"calc cryptogram");
	addr = fmBytes2intBE(addr_buf);    

	if(!fmBytes_compare(card_crypt,0,calc_crypt,0,fmBytes_get_length(calc_crypt))){
		ccb_set_PC(CCB,addr);
	}
	return 0;
}
int rsp_is_app_exsit_je(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg)
{
    u8 *str,*aid_str;
	int len,aid_len,addr;
    fmBytes *app_aid = arg->array[0];
    fmBytes *addr_buf = arg->array[1];
	fmBytes *resp = rsp->array[0];

    str = fmBytes_get_buf(resp);
	aid_str = fmBytes_get_buf(app_aid);
	len = fmBytes_get_length(resp);
	aid_len = fmBytes_get_length(app_aid);
	addr = fmBytes2intBE(addr_buf);    
	if(fm_strstr(str,len,aid_str,aid_len) != NULL){
		ccb_set_PC(CCB,addr);
	}
	return 0;
}

int rsp_is_personalized_je(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg)
{
    u8 *p,*end;
	int addr;
	fmBytes *addr_buf = arg->array[0];
	fmBytes *resp = rsp->array[0];
	
	addr = fmBytes2intBE(addr_buf);    
    p = fmBytes_get_buf(resp);
	end = p + fmBytes_get_length(resp);
	p ++;//skip A3
	(*p == 0xFF)?(p+=3):(p++);//skip len
	p += 3;//skip A2 len seq
    if(is_all_zero(p,end-p-2)){
		ccb_set_PC(CCB,addr);
    }
	return 0;
}
int rsp_is_card_active_je(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg)
{
	int addr,len0;
	u8 *str0;
	u8 ack[]={0x66,0x10};
	fmBytes *addr_buf = arg->array[0];
	fmBytes *resp = rsp->array[0];
	
	addr = fmBytes2intBE(addr_buf);    
    str0 = fmBytes_get_buf(resp);
	len0 = fmBytes_get_length(resp);
	if(!fm_strstr(str0,len0,ack,sizeof(ack))){
		ccb_set_PC(CCB,addr);
	}
	return 0;
}

#if 0
/*
                              ins(4)  parm0(lv)  parm1(lv) parm2(lv)
request for install param:    0       DB-ID       app_aid  seid
request for cap info:         1       DB-ID       seid
request for personalize data: 2       DB-ID       seid
*/
int db_request(void *CCB,fmBytes *rsp,fmBytes_array_t *arg)
{
    int i,pos;
	u8 *buf;
	fmBytes *seid= data_get_by_name(CCB,"seid");
	fmBytes *app_aid;
	fmBytes *apdu = fmBytes_alloc(MAX_APDU_LEN);
	fmBytes_array_t *apdus = fmBytes_array_alloc(1);
	fmBytes *out = ccb_get_out_data(CCB);

    pos = 0;
    buf[pos++] = TAG_A0;
	pos++;//len
	buf[pos++] = 0;//seq
    for(i = 0; i < arg->size; i++){
		pos += fmbytes_concat_with_len(buf+pos,arg->array[i]);
    }
	pos += fmbytes_concat_with_len(buf+pos,seid);
	fmBytes_set_length(apdu,pos);
	buf[1] = pos-2;
	apdus->array[0] = apdu;
	_gp_make_apdus(apdus,&out);
	ccb_set_dest(CCB,AG_FUNCTION);
	ccb_set_write_back(fm_true);
	return 0;
}


int kms_requset(void *CCB,fmBytes *rsp,fmBytes_array_t *arg)
{
    int i,instruction,pos;
	u8 *buf;
	fmBytes *seid= data_get_by_name(CCB,"seid");
	fmBytes *app_aid;
	fmBytes *apdu = fmBytes_alloc(MAX_APDU_LEN);
	fmBytes_array_t *apdus = fmBytes_array_alloc(1);
	fmBytes *out = ccb_get_out_data(CCB);

	instruction = fmBytes2intBE(arg->array[0]);

    pos = 0;
    buf[pos++] = TAG_A0;
	pos++;//len
	buf[pos++] = 0;//seq
    for(i = 0; i < arg->size; i++){
		pos += fmbytes_concat_with_len(buf+pos,arg->array[i]);
    }
	if(instruction == 0){
		pos += fmbytes_concat_with_len(buf+pos,rsp);
	}else{
	    pos += fmbytes_concat_with_len(buf+pos,out);
	}
	
	fmBytes_set_length(apdu,pos);
	buf[1] = pos-2;
	apdus->array[0] = apdu;
	_gp_make_apdus(apdus,&out);
	ccb_set_dest(CCB,AG_FUNCTION);
	ccb_set_write_back(fm_true);
	return 0;
}

#endif
