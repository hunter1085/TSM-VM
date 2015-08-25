#include "./common/fm_hashtable.h"
#include "./common/fm_skbuff.h"
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
LOCAL int _gp_make_apdu(u8 cla,u8 ins,u8 p1,u8 p2,fmBytes *data,struct sk_buff *out)
{
	int offset = 0;
	u8 *outbuf,buf;
	u8 len;
	if((out == NULL) || (out->data_len == 0)) 
		return FM_SYS_ERR_NULL_POINTER;
	if(data == NULL){
        len = 0;
	}else{
	    len = fmBytes_get_length(data);
	}
	buf = fmBytes_get_buf(data);
	if((len != 0)&&(buf == NULL)) 
		return FM_GEN_ERR_INVALID_LEN;

	
	*(skb_put(out,1)) = cla;
	*(skb_put(out,1)) = ins;
	*(skb_put(out,1)) = p1;
	*(skb_put(out,1)) = p2;
	*(skb_put(out,1)) = len;
	if(len != 0){
		memcpy(skb_put(out,len),buf,len);
	}
	return FM_OK;
} 

/*********************************************************************************
all the following gp cmd provides only the raw apdu,without le,without MAC
*********************************************************************************/
int _gp_select(fmBytes *aid,struct sk_buff *skb)
{
	return _gp_make_apdu(CLA_7816,GP_INS_SEL,0x04,0x00,aid,skb);
}
int _gp_init_update(u8 kv,fmBytes *hc,struct sk_buff *skb)
{
	return _gp_make_apdu(CLA_GP,GP_INS_INIT_UPDATE,kv,0x00,hc,skb);
}
int _gp_ext_auth(u8 sl,fmBytes *hc,struct sk_buff *skb)
{
	return _gp_make_apdu(CLA_GP,GP_INS_EXT_AUTH,sl,0x00,hc,skb);
}
int _gp_get_data(u8 tag_hi,u8 tag_lo,fmBytes *tag_list,struct sk_buff *skb)
{
    return _gp_make_apdu(CLA_GP,GP_INS_GET_DATA,tag_hi,tag_lo,tag_list,skb);
}
int _gp_delete(u8 more,u8 opt,fmBytes *tlvs,struct sk_buff *skb)
{
    return _gp_make_apdu(CLA_GP,GP_INS_DELETE,more,opt,tlvs,skb);
}
int _gp_get_status(u8 subsets,fmBytes *search_criteria,struct sk_buff *skb)
{
    return _gp_make_apdu(CLA_GP,GP_INS_GET_STS,subsets,0x02,search_criteria,skb);
}
int _gp_install(u8 p1,fmBytes *data,struct sk_buff *skb)
{
    return _gp_make_apdu(CLA_GP,GP_INS_INSTALL,p1,0x00,data,skb);
}
int _gp_load(u8 more,u8 blk_num,fmBytes *data,struct sk_buff *skb)
{
    return _gp_make_apdu(CLA_GP,GP_INS_LOAD,more,blk_num,data,skb);
}
int _gp_put_key(u8 kv,u8 kid,fmBytes *key_data,struct sk_buff *skb)
{
    return _gp_make_apdu(CLA_GP,GP_INS_PUT_KEY,kv,kid,key_data,skb);
}
int _gp_set_status(u8 domain,u8 status,fmBytes *aid,struct sk_buff *skb)
{
    return _gp_make_apdu(CLA_GP,GP_INS_SET_STS,domain,status,aid,skb);
}
int _gp_store_data(u8 p1,u8 blk_num,fmBytes *data,struct sk_buff *skb)
{
    return _gp_make_apdu(CLA_GP,GP_INS_STORE_DATA,p1,blk_num,data,skb);
}

int _gp_install_for_install_selectable(fmBytes *pkg_aid,fmBytes *app_aid,fmBytes *ins_aid,
	fmBytes *priv,fmBytes *install_parm,fmBytes *token,struct sk_buff *skb)
{
	int pos;
	u8 buf[MAX_APDU_LEN];
	fmBytes data;
	fmBytes_set_buf(&data,buf);
	fmBytes_set_size(&data,MAX_APDU_LEN);

	pos = 0;
	pos += fmbytes_concat_with_len(buf+pos,pkg_aid);
	pos += fmbytes_concat_with_len(buf+pos,app_aid);
	pos += fmbytes_concat_with_len(buf+pos,ins_aid);
	pos += fmbytes_concat_with_len(buf+pos,priv);
	pos += fmbytes_concat_with_len(buf+pos,install_parm);
	pos += fmbytes_concat_with_len(buf+pos,token);
	fmBytes_set_length(&data,pos);
    _gp_install(INSTALL_FOR_INSTALL | INSTALL_FOR_SELECT,&data,skb);
	return 0;
}

int _gp_install_for_personalize(fmBytes *aid,struct sk_buff *skb)
{
	int pos;
	u8 buf[MAX_APDU_LEN];
	fmBytes data;
	fmBytes_set_buf(&data,buf);
	fmBytes_set_size(&data,MAX_APDU_LEN);

	pos = 2;
	pos += fmbytes_concat_with_len(buf+pos,aid);
	pos += 3;
	fmBytes_set_length(&data,pos);
    _gp_install(INSTALL_FOR_PERSONIZE,&data,skb);
	return 0;
}

int _gp_install_for_load(fmBytes *pkg_aid,fmBytes *sd_aid,fmBytes *hash,
	fmBytes *load_parm,fmBytes *load_token,struct sk_buff *skb)
{
	int pos;
	u8 buf[MAX_APDU_LEN];
	fmBytes data;
	fmBytes_set_buf(&data,buf);
	fmBytes_set_size(&data,MAX_APDU_LEN);

	pos = 0;
	pos += fmbytes_concat_with_len(buf+pos,pkg_aid);
	pos += fmbytes_concat_with_len(buf+pos,sd_aid);
	pos += fmbytes_concat_with_len(buf+pos,hash);
	pos += fmbytes_concat_with_len(buf+pos,load_parm);
	pos += fmbytes_concat_with_len(buf+pos,load_token);
	fmBytes_set_length(&data,pos);
    _gp_install(INSTALL_FOR_PERSONIZE,&data,skb);
	return 0;
}

int _gp_install_for_extradition(fmBytes *dest_aid,fmBytes *app_aid,
	fmBytes *parm,fmBytes *token,struct sk_buff *skb)
{
	int pos;
	u8 buf[MAX_APDU_LEN];
	fmBytes data;
	fmBytes_set_buf(&data,buf);
	fmBytes_set_size(&data,MAX_APDU_LEN);

	pos = 0;
	pos += fmbytes_concat_with_len(buf+pos,dest_aid);
	pos ++;
	pos += fmbytes_concat_with_len(buf+pos,app_aid);
	pos ++;
	pos += fmbytes_concat_with_len(buf+pos,parm);
	pos += fmbytes_concat_with_len(buf+pos,token);
	fmBytes_set_length(&data,pos);
	_gp_install(INSTALL_FOR_EXTRA,&data,skb);
	return 0;
}

void _gp_padding_sw(struct sk_buff_head  *apdus)
{
	struct sk_buff	*pos,*n;

	list_for_each_safe(pos,n,apdus){
		*(skb_put(pos,1)) = 0x90;
		*(skb_put(pos,1)) = 0x00;
	}
}
int _gp_padding_mac(fmBytes_array_t *apdus)
{
    
}

struct sk_buff *apdu_preprocess(fmBytes *agent)
{
    int len;
    struct sk_buff *skb;

    skb = alloc_skb(MAX_APDU_LEN);
    len = fmBytes_get_length(agent);
	skb->agent = fmBytes_alloc(len);
	fmBytes_copy(skb->agent,0,agent,0,len);
	skb_reserve(skb,TAG_A0_HEAD_ROOM);
	return skb;
}
/*
A0/A1 macing is not added here
*/
int gp_select(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg)
{
    int ret;
    fmBytes *agent  = arg->array[0];
	fmBytes *aid  = arg->array[1];
	struct sk_buff *skb;
	struct sk_buff_head *cmdQ = ccb_get_cmdQ(CCB);

    skb_queue_purge(cmdQ);
    skb = apdu_preprocess(agent);
	_gp_select(aid,skb);
	skb_queue_tail(cmdQ,skb);
	_gp_padding_sw(cmdQ);
	ccb_set_write_back(fm_true);
	
	return ret;
}
int gp_init_update(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg)
{
    int ret;
    fmBytes *agent  = arg->array[0];
    fmBytes *kv = arg->array[1];
	fmBytes *hc = data_get_by_name(CCB,"host challenge");
	struct sk_buff *skb;
    struct sk_buff_head *cmdQ = ccb_get_cmdQ(CCB);

    skb_queue_purge(cmdQ);
	skb = apdu_preprocess(agent);
	_gp_init_update(kv->buf[0],hc,skb);
	skb_queue_tail(cmdQ,skb);
	_gp_padding_sw(cmdQ);
	ccb_set_write_back(fm_true);
	
	return ret;
}

/*
for the apdus that require mac,we cached them in the Queue,
while at the same time, these apdus are packet together and put
in the ccb->out,so that vm will fetch this data and send to the
agent
*/
int gp_ext_auth(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg)
{
    int ret;
	fmBytes *agent  = arg->array[0];
    fmBytes *sl = arg->array[1];
    fmBytes *nhc = data_get_by_name(CCB,"new host challenge");
	struct sk_buff *skb;
    struct sk_buff_head *apduQ = ccb_get_apduQ(CCB);

    skb_queue_purge(apduQ);
	skb = apdu_preprocess(agent);
	_gp_ext_auth(sl->buf[0],nhc,skb);
	skb_queue_tail(apduQ,skb);
	ccb_set_bootstrap(CCB,fm_true);
	return ret;
}

int gp_get_status(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg)
{
    int ret;
	fmBytes *agent  = arg->array[0];
    fmBytes *subset = arg->array[1];
	fmBytes *search_criteria = arg->array[2];
	struct sk_buff *skb;
    struct sk_buff_head *apduQ = ccb_get_apduQ(CCB);

    skb_queue_purge(apduQ);
	skb = apdu_preprocess(agent);
	_gp_get_status(subset->buf[0],search_criteria,skb);
	skb_queue_tail(apduQ,skb);
	ccb_set_bootstrap(CCB,fm_true);

	return ret;
}

int gp_install_for_install_make_selectable(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg)
{
    int ret;
	fmBytes *agent  = arg->array[0];
	fmBytes *pkg_aid = arg->array[1];
	fmBytes *app_aid = arg->array[2];
	fmBytes *ins_aid = arg->array[3];
	fmBytes *priv = arg->array[4];
	fmBytes *install_parm = data_get_by_name(CCB,"parameter");
	fmBytes *token = data_get_by_name(CCB,"token");
	struct sk_buff *skb;
    struct sk_buff_head *apduQ = ccb_get_apduQ(CCB);

    skb_queue_purge(apduQ);
    skb = apdu_preprocess(agent);
	_gp_install_for_install_selectable(pkg_aid,app_aid,ins_aid,priv,install_parm,token,skb);	
	skb_queue_tail(apduQ,skb);
	ccb_set_bootstrap(CCB,fm_true);
	return ret;
}
int gp_install_for_personalize(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg)
{
    int ret;
	fmBytes *agent  = arg->array[0];
	fmBytes *aid = arg->array[1];
	struct sk_buff *skb;
    struct sk_buff_head *apduQ = ccb_get_apduQ(CCB);

    skb_queue_purge(apduQ);
	skb = apdu_preprocess(agent);
    _gp_install_for_personalize(aid,skb);
    skb_queue_tail(apduQ,skb);
	ccb_set_bootstrap(CCB,fm_true);
	return ret;
}
int gp_put_key(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg)
{
    int ret;
	fmBytes *agent  = arg->array[0];
	fmBytes *kv = arg->array[1];
	struct sk_buff *skb;
    struct sk_buff_head *apduQ = ccb_get_apduQ(CCB);

    skb_queue_purge(apduQ);
	skb = apdu_preprocess(agent);
	_gp_put_key(kv->buf[0],0x81,NULL,skb);
    skb_queue_tail(apduQ,skb);
	ccb_set_bootstrap(CCB,fm_true);
	return ret;
}
int gp_install_for_load_and_load(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg)
{
    #define MAX_LOAD_DATA_SIZE  232
    int i,ret,berlen_len,cap_len;
	u8 pos = 0,block_num = 0,berlen[10] = {0},buf[MAX_APDU_LEN];
    fmBytes data;
	fmBytes *agent  = arg->array[0];
	fmBytes *sd_aid = arg->array[1];
	fmBytes *pkg_aid = data_get_by_name(CCB,"cap pkg aid");
	fmBytes *hash = data_get_by_name(CCB,"cap hash");
	fmBytes *load_parm = data_get_by_name(CCB,"load parameter");
	fmBytes *content = data_get_by_name(CCB,"cap content");
	fmBytes *load_token = data_get_by_name(CCB,"token");
	struct sk_buff *skb;
    struct sk_buff_head *apduQ = ccb_get_apduQ(CCB);
	
	skb_queue_purge(apduQ);
	cap_len    = fmBytes_get_length(content);

    /*install for load*/
	skb = apdu_preprocess(agent);
    _gp_install_for_load(pkg_aid,sd_aid,hash,load_parm,load_token,skb);
	skb_queue_tail(apduQ,skb);

	/*load*/
	skb = apdu_preprocess(agent);
	berlen_len = get_BERLen(content->leng,berlen);
	fmBytes_set_buf(&data,buf);
	fmBytes_set_size(&data,MAX_APDU_LEN);
	buf[pos++] = 0xC4;
	memcpy(buf+pos,berlen,berlen_len);
	pos += berlen_len;
	memcpy(buf+pos,fmBytes_get_buf(content),cap_len<288?cap_len:288);
	pos += cap_len<288?cap_len:288;
	fmBytes_set_length(&data,pos);
	_gp_load(cap_len<288?0x80:0,block_num++,&data,skb);
    skb_queue_tail(apduQ,skb);

	pos = cap_len<288?cap_len:288;
	cap_len -= pos;
	i = 2;
	while(cap_len > 0){
		skb = apdu_preprocess(agent);
		fmBytes_set_buf(&data,fmBytes_get_buf(content)+pos);
		if(cap_len > MAX_LOAD_DATA_SIZE){
			fmBytes_set_length(&data,MAX_LOAD_DATA_SIZE);
			_gp_load(0x80,block_num++,&data,skb);
			
			pos += MAX_LOAD_DATA_SIZE;
			cap_len -= pos;
		}else{
		    fmBytes_set_length(&data,cap_len);
			_gp_load(0x00,block_num++,&data,skb);
			pos += cap_len;
			cap_len -= pos;
		}
		skb_queue_tail(apduQ,skb);
	}
	ccb_set_bootstrap(CCB,fm_true);
	return 0;
}
int gp_install_for_extradition(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg)
{
    int ret;
	fmBytes *agent  = arg->array[0];
	fmBytes *dest_aid = arg->array[1];
	fmBytes *app_aid = arg->array[2];
	fmBytes *parm = arg->array[3];
	fmBytes *token = data_get_by_name(CCB,"token");
	struct sk_buff *skb;
    struct sk_buff_head *apduQ = ccb_get_apduQ(CCB);

    skb_queue_purge(apduQ);
	skb = apdu_preprocess(agent);
    _gp_install_for_extradition(dest_aid,app_aid,parm,token,skb);
	skb_queue_tail(apduQ,skb);
	ccb_set_bootstrap(CCB,fm_true);
	return ret;
}
/*
personalize data is store in the hash table,they are identified by arg->array[i]
*/
int gp_store_data(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg)
{
    int i,ret;
	u8 blk_num = 0x00;
	fmBytes *agent  = arg->array[0];
	struct sk_buff *skb;
    struct sk_buff_head *apduQ = ccb_get_apduQ(CCB);

    skb_queue_purge(apduQ);
    for(i = 1; i < arg->size; i++){
		fmBytes *parm;
		skb = apdu_preprocess(agent);
		parm = data_get_by_name(CCB,arg->array[i]);
		_gp_store_data((i==(arg->size-1))?0x80:0x00,blk_num++,parm,skb);
		skb_queue_tail(apduQ,skb);
    }
	ccb_set_bootstrap(CCB,fm_true);
	return 0;
}
int gp_traverse(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg)
{
    int i,len;
	u8 *buf;
	fmBytes *agent  = arg->array[0];
	struct sk_buff *skb;
    struct sk_buff_head *cmdQ = ccb_get_cmdQ(CCB);
	
    skb_queue_purge(cmdQ);
	for(i = 1; i < arg->size; i++){
		buf = fmBytes_get_buf(arg->array[i]);
		len = fmBytes_get_length(arg->array[i]);
		skb = apdu_preprocess(agent);
		memcpy(skb_put(skb,len),buf,len);
		skb_queue_tail(cmdQ,skb);
		_gp_padding_sw(cmdQ);
	}
	ccb_set_write_back(fm_true);
	return 0;
}
int gp_msg_macing(void *CCB,fmBytes_array_t *rsp,fmBytes_array_t *arg)
{
	ccb_set_write_back(fm_true);
	return 0;
}