#include "./common/fm_Base.h"
#include "./common/fm_List.h"
#include "bc.h"
#include "tsm_ccb.h"
#include "tsm_router.h"
#include "tsm_scloader.h"
#include "tsm_vm.h"

#define DEFAULT_RSP_FRAG_NUM        16

/*
all agents should have an entry in the agent_list of each CCB,except
the agent0,in such case we should create a new CCB
*/
LOCAL void *fetch_ccb(list_t *ccb_list,int session)
{
    struct list_head *pos,*n;
	void *CCB;
	
    LOCK(ccb_list->lock);
    list_for_each_safe(pos,n,&ccb_list->head){
		CCB = (void *)pos; 
		if(ccb_get_session(CCB) == session){
			UNLOCK(ccb_list->lock);
			return CCB;
		}
    }
	UNLOCK(ccb_list->lock);
	return NULL;
}


/*
padding each apdu in the apduQ,and return the total length of 
these apdus
*/
LOCAL int padding_tagA0(struct sk_buff_head *apduQ)
{
    u8 seq = 0x01;
	int len = 0;
	struct sk_buff *pos,*n;

	list_for_each_safe(pos,n,apduQ)
	{
		*(skb_push(pos,1)) = seq++;
		*(skb_push(pos,1)) = pos->len;
		*(skb_push(pos,1)) = TAG_A0;
		len += pos->len;
	}

    return len;
}



LOCAL void setting_agent_info(struct sk_buff *out,struct sk_buff_head *apduQ)
{
    int len;
    struct sk_buff *skb;

	skb = skb_peek(apduQ);
	len = fmBytes_get_length(skb->agent);
	out->agent = fmBytes_alloc(len);
	fmBytes_copy(out->agent,0,skb->agent,0,len);
}

LOCAL void padding_tagA1(struct sk_buff *out,struct sk_buff_head *cmdQ)
{
	struct sk_buff *pos,*n;
    skb_reserve(out,TAG_A1_HEAD_ROOM);

    list_for_each_safe(pos,n,cmdQ){
		memcpy(skb_put(out,pos->len),pos->data,pos->len);
    }	

	
	if(out->len > 255){
		*(skb_put(out,1)) = (out->len&0xFF);
		*(skb_put(out,1)) = (out->len&0xFF00)>>8;
		*(skb_put(out,1)) = 0xFF;
	}else{
	    *(skb_put(out,1)) = (out->len&0xFF);
	}
	*(skb_put(out,1)) = TAG_A1;
}
LOCAL void skip_tagAx(struct sk_buff *skb)
{
    skb_pull(skb,1);
	if(*skb_pull(skb,1) == 0xFF){
		skb_pull(skb,2);
	}
}
LOCAL fmBytes_array_t *tagA0s_to_fmBytesarray(struct sk_buff *skb)
{
    int i = 0;
    u8 len;
	fmBytes *frag;
	fmBytes_array_t *fba;

	fba = fmBytes_array_alloc(DEFAULT_RSP_FRAG_NUM);
	if(!fba)return NULL;
	
    while(skb->len > 0){
		skb_pull(skb,1);//tagA0
		len = *(skb_pull(skb,1));
		skb_pull(skb,1);//seq
		frag = fmBytes_alloc(len-1);
		fmBytes_copy_from(frag,0,skb_pull(skb,len-1),0,len-1);
		if((fba->size-fba->cnt)<=1){
			fba = fmBytes_array_resize(DEFAULT_RSP_FRAG_NUM<<1);
		}
		fba->array[i++] = frag;
		fba->cnt = i;
    }
	return fba;
}
PUBLIC void *tsm_vm(void* para)
{
    int len;
	code_entry_t *ce;
	byte_code_t *bc_array;
	script_entry_t *se;
	void *CCB;
	tsm_dev_t *tsm_dev = (tsm_dev_t *)para;
	tsm_router_t *router = tsm_dev->g_router;
	list_t *ccb_list = &tsm_dev->ccb_list;
	struct sk_buff *skb,*out;
	struct sk_buff_head *apduQ,*cmdQ;
	fmBytes_array_t *fba;
	while(1){
        sem_wait(&tsm_dev->sem_vm);
		skb = skb_dequeue(&router->rpkg_list);
		skip_tagAx(skb);
		if(skb->isNew){
		    fba = tagA0s_to_fmBytesarray(skb);
			skb->isNew = fm_false;
		}
		//???when skb & fba free?
		CCB = fetch_ccb(ccb_list,skb->session);
		if(CCB == NULL){
		}
		se = ccb_get_script(CCB);
		ce = se->ces[ccb_get_PC(CCB)];
		bc_array = ccb_get_bytecode(CCB);
		bc_array->bc[ce->code](CCB,fba,&ce->parameters);
		if(ccb_get_write_back(CCB)){/*dequeue from apduQ and pkg them with A0 and A1*/
			/*
			each time when we need write back,we know that the previous skb and fba
			have arrived the end of their lifetime,we should free them
			*/
			kfree_skb(skb);
			fmBytes_array_free(fba);

			ccb_set_write_back(CCB,fm_false);
			//apduQ = ccb_get_apduQ(CCB);
			cmdQ = ccb_get_cmdQ(CCB);
			len = padding_tagA0(cmdQ);
			//len += padding_tagA0(apduQ);
			out = alloc_skb(len+MSG_HEAD_LEN+TAG_A1_HEAD_ROOM);
            setting_agent_info(out,cmdQ);
			out->session = ccb_get_session(CCB);
			out->msg_type= MSG_DATA;
			
			skb_reserve(out,MSG_HEAD_LEN);
            padding_tagA1(out,cmdQ);

			skb_queue_tail(&router->wpkg_list,out);
			sem_post(&tsm_dev->sem_router_w);
		}
		if(ccb_get_bootstrap(CCB)){
			ccb_set_bootstrap(CCB,fm_false);
			skb->reason = PKG_REASON_BOOTSTRAP;
			skb_queue_tail(&router->rpkg_list,skb);
			sem_post(&tsm_dev->sem_vm);
		}
		
	}
	return (void *)0;
}
