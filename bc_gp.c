#include "./common/fm_hashtable.h"
#include "tsm_Err.h"
#include "bc.h"
#include "bc_gp.h"

#define CLA_7816				0x00
#define CLA_GP					0x80

#define SEC_OPT_NO_SEC				0x00
#define SEC_OPT_GP_SEC				0x04
#define SEC_OPT_7816_SEC_NO_CMAC	0x08
#define SEC_OPT_7816_SEC_CMAC		0x0C

#define GP_INS_SEL              0xA4
#define GP_INS_INIT_UPDATE      0x50
#define GP_INS_EXT_AUTH         0x82
#define GP_INS_INSTALL          0xE6
#define GP_INS_LOAD             0xE8
#define GP_INS_STORE_DATA       0xE2
#define GP_INS_GET_DATA         0xCA
#define GP_INS_GET_STS          0xF2
#define GP_INS_SET_STS          0xF0
#define GP_INS_DELETE           0xE4
#define GP_INS_PUT_KEY          0xD8

#define INSTALL_FOR_LOAD        0x02
#define INSTALL_FOR_INSTALL     0x04
#define INSTALL_FOR_SELECT      0x08
#define INSTALL_FOR_EXTRA       0x10
#define INSTALL_FOR_PERSONIZE   0x20
#define INSTALL_FOR_REG_UPDATE  0x40

#define SCP02_KEY_DIVERS_DATA_LEN   10
#define SCP02_KEY_INFO_LEN          2
#define SCP02_SEQ_COUNTER_LEN       2
#define SCP02_CARD_CHALLENGE_LEN    6
#define SCP02_CARD_CRYPTO_LEN       8
#define SCP02_VALID_DATA    (SCP02_KEY_DIVERS_DATA_LEN + \
	                            SCP02_KEY_INFO_LEN +\
	                            SCP02_SEQ_COUNTER_LEN+\
	                            SCP02_CARD_CHALLENGE_LEN+\
	                            SCP02_CARD_CRYPTO_LEN)

#define SCP02_SEQ_COUNTER_OFFSET    12
#define SCP02_CARD_CHALLENGE_OFFSET 14
#define SCP02_CARD_CRYPTO_OFFSET    20

/*
_gp_make_apdu make the apdu as the following format:
A0 len seq apdu
*/
LOCAL int _gp_make_apdu(u8 seq,u8 cla,u8 ins,u8 p1,u8 p2,fmBytes *data,fmBytes *out)
{
	int offset = 0;
	u8 *outbuf;
	u8 len;
	if((out == NULL) || (fmBytes_get_buf(out) == NULL) || (fmBytes_get_size(out) == 0)) 
		return FM_SYS_ERR_NULL_POINTER;
	if(data == NULL){
        len = 0;
	}else{
	    len = fmBytes_get_length(data);
	}
	if((fmBytes_get_length(data) != 0)&&(fmBytes_get_buf(data) == NULL)) 
		return FM_GEN_ERR_INVALID_LEN;

    outbuf = fmBytes_get_buf(out);
	outbuf[offset++] = TAG_A0;
	outbuf[offset++] = 0;//reserve for len
	outbuf[offset++] = seq;
	outbuf[offset++] = cla;
	outbuf[offset++] = ins;
	outbuf[offset++] = p1;
	outbuf[offset++] = p2;
	outbuf[offset++] = len;
	if(len != 0){
	    memcpy(outbuf+offset,fmBytes_get_buf(data),len); 
	}
	offset += len;
	fmBytes_set_length(out,offset);
	outbuf[1] = fmBytes_get_length(out)-2;
	return FM_OK;
} 

/*********************************************************************************
all the following gp cmd provides only the raw apdu,without le,without MAC
*********************************************************************************/
int _gp_select(u8 seq,fmBytes *aid,fmBytes *apdu_out)
{
	return _gp_make_apdu(seq,CLA_7816,GP_INS_SEL,0x04,0x00,aid,apdu_out);
}
int _gp_init_update(u8 seq,u8 kv,fmBytes *hc,fmBytes *apdu_out)
{
	return _gp_make_apdu(seq,CLA_GP,GP_INS_INIT_UPDATE,kv,0x00,hc,apdu_out);
}
int _gp_ext_auth(u8 seq,u8 sl,fmBytes *hc,fmBytes *apdu_out)
{
	return _gp_make_apdu(seq,CLA_GP,GP_INS_EXT_AUTH,sl,0x00,hc,apdu_out);
}
int _gp_get_data(u8 seq,u8 tag_hi,u8 tag_lo,fmBytes *tag_list,fmBytes *apdu_out)
{
    return _gp_make_apdu(seq,CLA_GP,GP_INS_GET_DATA,tag_hi,tag_lo,tag_list,apdu_out);
}
int _gp_delete(u8 seq,u8 more,u8 opt,fmBytes *tlvs,fmBytes *apdu_out)
{
    return _gp_make_apdu(seq,CLA_GP,GP_INS_DELETE,more,opt,tlvs,apdu_out);
}
int _gp_get_status(u8 seq,u8 subsets,fmBytes *search_criteria,fmBytes *apdu_out)
{
    return _gp_make_apdu(seq,CLA_GP,GP_INS_GET_STS,subsets,0x02,search_criteria,apdu_out);
}
int _gp_install(u8 seq,u8 p1,fmBytes *data,fmBytes *apdu_out)
{
    return _gp_make_apdu(seq,CLA_GP,GP_INS_INSTALL,p1,0x00,data,apdu_out);
}
int _gp_load(u8 seq,u8 more,u8 blk_num,fmBytes *data,fmBytes *apdu_out)
{
    return _gp_make_apdu(seq,CLA_GP,GP_INS_LOAD,more,blk_num,data,apdu_out);
}
int _gp_put_key(u8 seq,u8 kv,u8 kid,fmBytes *key_data,fmBytes *apdu_out)
{
    return _gp_make_apdu(seq,CLA_GP,GP_INS_PUT_KEY,kv,kid,key_data,apdu_out);
}
int _gp_set_status(u8 seq,u8 domain,u8 status,fmBytes *aid,fmBytes *apdu_out)
{
    return _gp_make_apdu(seq,CLA_GP,GP_INS_SET_STS,domain,status,aid,apdu_out);
}
int _gp_store_data(u8 seq,u8 p1,u8 blk_num,fmBytes *data,fmBytes *apdu_out)
{
    return _gp_make_apdu(seq,CLA_GP,GP_INS_STORE_DATA,p1,blk_num,data,apdu_out);
}

int _gp_install_for_install_selectable(u8 seq,fmBytes *pkg_aid,fmBytes *app_aid,fmBytes *ins_aid,
	fmBytes *priv,fmBytes *install_parm,fmBytes *token,fmBytes *out)
{
	int pos;
	fmBytes *data = fmBytes_alloc(MAX_APDU_LEN);
	u8 *buf = fmBytes_get_buf(data);

	pos = 0;
	pos += fmbytes_concat_with_len(buf+pos,pkg_aid);
	pos += fmbytes_concat_with_len(buf+pos,app_aid);
	pos += fmbytes_concat_with_len(buf+pos,ins_aid);
	pos += fmbytes_concat_with_len(buf+pos,priv);
	pos += fmbytes_concat_with_len(buf+pos,install_parm);
	pos += fmbytes_concat_with_len(buf+pos,token);
	fmBytes_set_length(data,pos);
    _gp_install(seq,INSTALL_FOR_INSTALL | INSTALL_FOR_SELECT,data,out);
	return 0;
}

int _gp_install_for_personalize(u8 seq,fmBytes *aid,fmBytes *out)
{
	int pos;
	fmBytes *data = fmBytes_alloc(MAX_APDU_LEN);
	u8 *buf = fmBytes_get_buf(data);

	pos = 2;
	pos += fmbytes_concat_with_len(buf+pos,aid);
	pos += 3;
	fmBytes_set_length(data,pos);
    _gp_install(seq,INSTALL_FOR_PERSONIZE,data,out);
	return 0;
}

int _gp_install_for_load(u8 seq,fmBytes *pkg_aid,fmBytes *sd_aid,fmBytes *hash,
	fmBytes *load_parm,fmBytes *load_token,fmBytes *out)
{
	int pos;
	fmBytes *data = fmBytes_alloc(MAX_APDU_LEN);
	u8 *buf = fmBytes_get_buf(data);

	pos = 0;
	pos += fmbytes_concat_with_len(buf+pos,pkg_aid);
	pos += fmbytes_concat_with_len(buf+pos,sd_aid);
	pos += fmbytes_concat_with_len(buf+pos,hash);
	pos += fmbytes_concat_with_len(buf+pos,load_parm);
	pos += fmbytes_concat_with_len(buf+pos,load_token);
	fmBytes_set_length(data,pos);
    _gp_install(seq,INSTALL_FOR_PERSONIZE,data,out);
	return 0;
}

int _gp_install_for_extradition(u8 seq,fmBytes *dest_aid,fmBytes *app_aid,
	fmBytes *parm,fmBytes *token,fmBytes *out)
{
	int pos;
	fmBytes *data = fmBytes_alloc(MAX_APDU_LEN);
	u8 *buf = fmBytes_get_buf(data);

	pos = 0;
	pos += fmbytes_concat_with_len(buf+pos,dest_aid);
	pos ++;
	pos += fmbytes_concat_with_len(buf+pos,app_aid);
	pos ++;
	pos += fmbytes_concat_with_len(buf+pos,parm);
	pos += fmbytes_concat_with_len(buf+pos,token);
	fmBytes_set_length(data,pos);
	_gp_install(seq,INSTALL_FOR_EXTRA,data,out);
	return 0;
}

int _gp_padding_sw(fmBytes_array_t *apdus)
{
    int i,len,size;
	fmBytes *apdu;
    u8 *end;
	

	for(i = 0; i < apdus->size; i++){
		apdu = apdus->array[i];
		size = fmBytes_get_size(apdu);
		len  = fmBytes_get_length(apdu);
		if((size-len)<2) 
			return FM_SYS_ERR_NO_MEM;

        end = fmBytes_get_buf(apdu) + len;
		*end++ = 0x90;
		*end++ = 0x00;
		fmBytes_set_length(apdu,len+2);
	}
	return FM_OK;
}
int _gp_padding_mac(fmBytes_array_t *apdus)
{
    
}
void _gp_make_apdus(fmBytes_array_t *apdus,fmBytes **out)
{
    int i,len = 0;
	fmBytes *apdu;
	fmBytes *msg;
	u8 *p;

	for(i = 0; i < apdus->size; i++){
		apdu = apdus->array[i];
		len += apdu->leng;
	}
	
	msg = fmBytes_alloc(len+4);//in case len>255,3 bytes is needed to represent it
	p = fmBytes_get_buf(msg);
	*p++ = TAG_A1;
	if(len > 255){
		*p++ = 0xFF;
		*p++ = (len&0xFF00)>>8;
		*p++ = (len&0xFF);
	}else{
	    *p++ = (len&0xFF);
	}
	for(i = 0; i < apdus->size; i++){
		apdu = apdus->array[i];
		len = fmBytes_get_length(apdu);
		memcpy(p,fmBytes_get_buf(apdu),len);
		p += len;
	}
	*out = msg;
}
/********************************************************************************
A0 contains a single APDU,A1 contains various of A1.
A0 len(1/3) seq data 
A1 len(1/3) data 
********************************************************************************/
int gp_select(void *CCB,fmBytes *rsp,fmBytes_array_t *arg)
{
    int ret;
    u8 seq = 0x01;
    fmBytes *aid  = arg->array[0];
	fmBytes *out = ccb_get_out_data(CCB);
	fmBytes *apdu;
	fmBytes_array_t *apdus = fmBytes_array_alloc(1);

    apdu = fmBytes_alloc(MAX_APDU_LEN);
	ret = _gp_select(seq,aid,apdu);
	if(ret != FM_OK) return ret;
	apdus->array[0] = apdu;
	_gp_padding_sw(apdus);
	_gp_make_apdus(apdus,&out);
	ccb_set_dest(CCB,AG_BUSINESS);
	ccb_set_write_back(fm_true);
	
	return ret;
}
int gp_init_update(void *CCB,fmBytes *rsp,fmBytes_array_t *arg)
{
    int ret;
    u8 seq = 0x01;
    fmBytes *kv  = arg->array[0];
	fmBytes *out = ccb_get_out_data(CCB);
	fmBytes *apdu,*hc=data_get_by_name(CCB,"host challenge");
	fmBytes_array_t *apdus = fmBytes_array_alloc(1);

    apdu = fmBytes_alloc(MAX_APDU_LEN);
	ret = _gp_init_update(seq,kv->buf[0],hc,apdu);
	if(ret != FM_OK) return ret;
	apdus->array[0] = apdu;
	_gp_padding_sw(apdus);
	_gp_make_apdus(apdus,&out);
	ccb_set_dest(CCB,AG_BUSINESS);
	ccb_set_write_back(fm_true);
	return ret;
}
int gp_ext_auth(void *CCB,fmBytes *rsp,fmBytes_array_t *arg)
{
    int ret;
	u8 seq = 0x01;
    fmBytes *sl = arg->array[0];
	fmBytes *apdu,*nhc=data_get_by_name(CCB,"new host challenge");;
	fmBytes *out = ccb_get_out_data(CCB);
	fmBytes_array_t *apdus = fmBytes_array_alloc(1);

	apdu = fmBytes_alloc(MAX_APDU_LEN);
	ret = _gp_ext_auth(seq,sl->buf[0],nhc,apdu);
	if(ret != FM_OK) return ret;
	apdus->array[0] = apdu;
	ccb_set_apdus(CCB,apdus);
	_gp_make_apdus(apdus,&out);
	ccb_set_bootstrap(CCB,fm_true);
	return ret;
}

int gp_get_status(void *CCB,fmBytes *rsp,fmBytes_array_t *arg)
{
    int ret;
	u8 seq = 0x01;
    fmBytes *subset = arg->array[0];
	fmBytes *search_criteria = arg->array[1];
	fmBytes *out = ccb_get_out_data(CCB);
	fmBytes *apdu;
	fmBytes_array_t *apdus = fmBytes_array_alloc(1);

	apdu = fmBytes_alloc(MAX_APDU_LEN);
	ret = _gp_get_status(seq,subset->buf[0],search_criteria,apdu);
	if(ret != FM_OK) return ret;
	apdus->array[0] = apdu;
	ccb_set_apdus(CCB,apdus);
	_gp_make_apdus(apdus,&out);
	ccb_set_bootstrap(CCB,fm_true);
	return ret;
}

int gp_install_for_install_make_selectable(void *CCB,fmBytes *rsp,fmBytes_array_t *arg)
{
    int ret;
	u8 seq = 0x01;
	fmBytes *apdu;
	fmBytes *install_parm = data_get_by_name(CCB,"parameter");
	fmBytes *token = data_get_by_name(CCB,"token");
	fmBytes_array_t *apdus = fmBytes_array_alloc(1);
	fmBytes *pkg_aid = arg->array[0];
	fmBytes *app_aid = arg->array[1];
	fmBytes *ins_aid = arg->array[2];
	fmBytes *priv = arg->array[3];
	fmBytes *out = ccb_get_out_data(CCB);

	apdu = fmBytes_alloc(MAX_APDU_LEN);
	_gp_install_for_install_selectable(seq,pkg_aid,app_aid,ins_aid,priv,install_parm,token,apdu);
	if(ret != FM_OK) return ret;
	apdus->array[0] = apdu;
	ccb_set_apdus(CCB,apdus);
	_gp_make_apdus(apdus,&out);
	ccb_set_bootstrap(CCB,fm_true);
	return ret;
}
int gp_install_for_personalize(void *CCB,fmBytes *rsp,fmBytes_array_t *arg)
{
    int ret;
	u8 seq = 0x01;
	fmBytes *apdu;
	fmBytes_array_t *apdus = fmBytes_array_alloc(1);
	fmBytes *aid = arg->array[0];
	fmBytes *out = ccb_get_out_data(CCB);

	apdu = fmBytes_alloc(MAX_APDU_LEN);
    _gp_install_for_personalize(seq,aid,apdu);
	if(ret != FM_OK) return ret;
	apdus->array[0] = apdu;
	ccb_set_apdus(CCB,apdus);
	_gp_make_apdus(apdus,&out);
	ccb_set_bootstrap(CCB,fm_true);
	return ret;
}
int gp_put_key(void *CCB,fmBytes *rsp,fmBytes_array_t *arg)
{
    int ret;
	u8 seq = 0x01;
	fmBytes *apdu;
	fmBytes *kv = arg->array[0];
	fmBytes *out = ccb_get_out_data(CCB);
	fmBytes_array_t *apdus = fmBytes_array_alloc(1);

	apdu = fmBytes_alloc(MAX_APDU_LEN);
	_gp_put_key(seq,kv->buf[0],0x81,NULL,apdu);
	if(ret != FM_OK) return ret;
	apdus->array[0] = apdu;
	ccb_set_apdus(CCB,apdus);
	_gp_make_apdus(apdus,&out);
	ccb_set_bootstrap(CCB,fm_true);
	return ret;
}
int gp_install_for_load_and_load(void *CCB,fmBytes *rsp,fmBytes_array_t *arg)
{
    #define MAX_LOAD_DATA_SIZE  232
    int i,ret,berlen_len,cap_len;
	u8 pos = 0,seq = 0x01,block_num = 0,berlen[10] = {0};
	u8 *buf;
	fmBytes *apdu,*data;
	fmBytes_array_t *apdus;
	fmBytes *out = ccb_get_out_data(CCB);
	fmBytes *sd_aid = arg->array[0];
	fmBytes *pkg_aid = data_get_by_name(CCB,"cap pkg aid");
	fmBytes *hash = data_get_by_name(CCB,"cap hash");
	fmBytes *load_parm = data_get_by_name(CCB,"load parameter");
	fmBytes *content = data_get_by_name(CCB,"cap content");
	fmBytes *load_token = data_get_by_name(CCB,"token");
	
	
	cap_len    = fmBytes_get_length(content);
	apdus = fmBytes_array_alloc(cap_len/MAX_LOAD_DATA_SIZE + 2);

    /*install for load*/
    apdu = fmBytes_alloc(MAX_APDU_LEN);
    _gp_install_for_load(seq++,pkg_aid,sd_aid,hash,load_parm,load_token,apdu);
	if(ret != FM_OK) return ret;
	apdus->array[0] = apdu;

	/*load*/
	berlen_len = get_BERLen(content->leng,berlen);
	apdu = fmBytes_alloc(MAX_APDU_LEN);
	data = fmBytes_alloc(MAX_APDU_LEN);
	buf = fmBytes_get_buf(data);
	buf[pos++] = 0xC4;
	memcpy(buf+pos,berlen,berlen_len);
	pos += berlen_len;
	memcpy(buf+pos,fmBytes_get_buf(content),cap_len<288?cap_len:288);
	pos += cap_len<288?cap_len:288;
	fmBytes_set_length(data,pos);
	_gp_load(seq++,cap_len<288?0x80:0,block_num++,data,apdu);
	apdus->array[1] = apdu;

	pos = cap_len<288?cap_len:288;
	cap_len -= pos;
	i = 2;
	while(cap_len > 0){
		apdu = fmBytes_alloc(MAX_APDU_LEN);
		fmBytes_set_buf(data,fmBytes_get_buf(content)+pos);
		if(cap_len > MAX_LOAD_DATA_SIZE){
			fmBytes_set_length(data,MAX_LOAD_DATA_SIZE);
			_gp_load(seq++,0x80,block_num++,data,apdu);
			
			pos += MAX_LOAD_DATA_SIZE;
			cap_len -= pos;
		}else{
		    fmBytes_set_length(data,cap_len);
			_gp_load(seq++,0x00,block_num++,data,apdu);
			pos += cap_len;
			cap_len -= pos;
		}
		apdus->array[i++] = apdu;
	}
	ccb_set_apdus(CCB,apdus);
	_gp_make_apdus(apdus,&out);
	ccb_set_bootstrap(CCB,fm_true);
	return 0;
}
int gp_install_for_extradition(void *CCB,fmBytes *rsp,fmBytes_array_t *arg)
{
    int ret;
	u8 seq = 0x01;
	fmBytes_array_t *apdus = fmBytes_array_alloc(1);
	fmBytes *dest_aid = arg->array[0];
	fmBytes *app_aid = arg->array[1];
	fmBytes *parm = arg->array[2];
	fmBytes *out = ccb_get_out_data(CCB);
	fmBytes *token = data_get_by_name(CCB,"token");
	fmBytes *apdu = fmBytes_alloc(MAX_APDU_LEN);
	
    _gp_install_for_extradition(seq,dest_aid,app_aid,parm,token,apdu);
	if(ret != FM_OK) return ret;
	apdus->array[0] = apdu;
	ccb_set_apdus(CCB,apdus);
	_gp_make_apdus(apdus,&out);
	ccb_set_bootstrap(CCB,fm_true);
	return ret;
}
/*
personalize data is store in the hash table,they are identified by arg->array[i]
*/
int gp_store_data(void *CCB,fmBytes *rsp,fmBytes_array_t *arg)
{
    int i,ret;
	u8 seq = 0x01,blk_num = 0x00;
	fmBytes_array_t *apdus;
    fmBytes *apdu;
	fmBytes *out = ccb_get_out_data(CCB);

    for(i = 0; i < arg->size; i++){
		fmBytes *parm;
		parm = data_get_by_name(CCB,arg->array[i]);
		_gp_store_data(seq++,(i==(arg->size-1))?0x80:0x00,blk_num++,parm,apdu);
		apdus->array[i] = apdu;
    }

	ccb_set_apdus(CCB,apdus);
	_gp_make_apdus(apdus,&out);
	ccb_set_bootstrap(CCB,fm_true);
	return 0;
}
int gp_traverse(void *CCB,fmBytes *rsp,fmBytes_array_t *arg)
{
    int i,num;
	fmBytes_array_t *apdus;
    fmBytes *raw;
	fmBytes *out = ccb_get_out_data(CCB);

	num = arg->size;
	apdus = fmBytes_array_alloc(num);
	for(i = 0; i < num; i++){
		raw = fmBytes_alloc(MAX_APDU_LEN);
		fmBytes_copy(raw,0,arg->array[i],0,fmBytes_get_length(arg->array[i]));
		apdus->array[i] = raw;
	}
	_gp_padding_sw(apdus);
	_gp_make_apdus(apdus,&out);
	ccb_set_dest(CCB,AG_BUSINESS);
	ccb_set_write_back(fm_true);
	return 0;
}
int gp_msg_macing(void *CCB,fmBytes *rsp,fmBytes_array_t *arg)
{
    fmBytes *out = ccb_get_out_data(CCB);
	out = rsp;
	ccb_set_dest(CCB,AG_BUSINESS);
	ccb_set_write_back(fm_true);
	return 0;
}