#include "tsm_vm.h"
#include "tsm_bcloader.h"
#include "tsm_ccb.h"

#define HASH_TABLE_DEFAULT_SIZE     64
#define BYTE_CODE_DEFAULT_SIZE      512

void ccb_bre_entry_value_del(fmBytes *v);
/*it is the inner byte code for reserve,
  as it does not belong to any byte code class,
  it is implemented here*/
int tsm_reserve()
{
    return 0;
}

void ccb_add_bc(bc_set_t *bcs,byte_code_t *bc)
{
    tsm_bytecode *p;
	int cnt;

    cnt = bc->cnt;
	while((bcs->base+bcs->cnt)>cnt){
		cnt += (cnt>>1);/////1.5
	}
	p = (tsm_bytecode *)fm_calloc(cnt,sizeof(tsm_bytecode));
	memset_int(p,tsm_reserve,cnt);
	memcpy_int(p,bc->bc,bc->cnt);
    memcpy_int(p+bc->cnt*sizeof(tsm_bytecode),bcs->bc,bcs->cnt);
	fm_free(bc->bc);
    bc->bc = p;
	bc->cnt = cnt;
}
byte_code_t *ccb_alloc_bc()
{
    byte_code_t *bc;
	bc = (byte_code_t *)fm_calloc(1,sizeof(byte_code_t));
	if(!bc) return NULL;
	bc->cnt = BYTE_CODE_DEFAULT_SIZE;
	bc->bc  = (tsm_bytecode *)fm_calloc(BYTE_CODE_DEFAULT_SIZE,sizeof(tsm_bytecode));
	memset_int(bc->bc,tsm_reserve,BYTE_CODE_DEFAULT_SIZE);
	return bc;
}
tsm_ccb_t *ccb_create_ccb(list_t *bc_list,list_t *tlv)
{
    char *dl;
    int i;
	tsm_ccb_t *pCCB;
	struct list_head *pos,*n,*pos1,*n1;
	bc_set_t *temp_set;
	fmBool found;
	router_tlv_t *node;
		
	pCCB = (tsm_ccb_t *)fm_calloc(1,sizeof(tsm_ccb_t));
	if(!pCCB){
		return NULL;
	}
	
	pCCB->bre = zc_hashtable_new(HASH_TABLE_DEFAULT_SIZE,
		            zc_hashtable_str_hash,
		            zc_hashtable_str_equal,
		            NULL,
		            (zc_hashtable_del_fn)ccb_bre_entry_value_del);
    
	list_for_each_safe(pos, n,&tlv->head){
		node = (router_tlv_t *)pos;
		if(node->tag == TAG_SESSION){
			pCCB->session = atoi(node->val);
			break;
		}
	}
	list_for_each_safe(pos, n,&tlv->head){
		node = (router_tlv_t *)pos;
		if(node->tag == TAG_SCRIPT){
			pCCB->se = scloader_load(node->val);
			break;
		}
	}
	
	pCCB->bc = ccb_alloc_bc();
    list_for_each_safe(pos, n,&tlv->head){
		found = fm_false;
		node = (router_tlv_t *)pos;
		if(node->tag == TAG_DLL){
			list_for_each_safe(pos1, n1, &bc_list->head){
				temp_set = (bc_set_t *)pos1;
				if(strcmp(temp_set->name,dl) == 0){
					found = fm_true;
					break;
				}
			}
    		if(!found){
    		    temp_set = bcloader_load(dl,bc_list);
    		}
    		if(!temp_set){
    		    ccb_add_bc(temp_set,pCCB->bc);
    		}
		}
    }
	return pCCB;
}



int ccb_get_PC(void *ccb)
{
    tsm_ccb_t *pCCB = (tsm_ccb_t *)ccb;
	return pCCB->PC;
}
void ccb_set_PC(void *ccb,int addr)
{
    tsm_ccb_t *pCCB = (tsm_ccb_t *)ccb;
	pCCB->PC = addr;
}
script_entry_t *ccb_get_script(void *ccb)
{
    tsm_ccb_t *pCCB = (tsm_ccb_t *)ccb;
	return pCCB->se;
}
void ccb_set_script(void *ccb,script_entry_t *se)
{
    tsm_ccb_t *pCCB = (tsm_ccb_t *)ccb;
	pCCB->se = se;
}
fmBool ccb_get_write_back(void *ccb)
{
    tsm_ccb_t *pCCB = (tsm_ccb_t *)ccb;
	return pCCB->write_back;
}
void ccb_set_write_back(void *ccb,int result)
{
    tsm_ccb_t *pCCB = (tsm_ccb_t *)ccb;
	pCCB->write_back = result;
}
fmBool ccb_get_bootstrap(void *ccb)
{
    tsm_ccb_t *pCCB = (tsm_ccb_t *)ccb;
	return pCCB->bootstrap;
}
void ccb_set_bootstrap(void *ccb,int result)
{
    tsm_ccb_t *pCCB = (tsm_ccb_t *)ccb;
	pCCB->bootstrap = result;
}
pkg_entry_t *ccb_get_out_pkg(void *ccb)
{
    tsm_ccb_t *pCCB = (tsm_ccb_t *)ccb;
	return pCCB->out;
}
void ccb_set_out_pkg(void *ccb,pkg_entry_t *out)
{
    tsm_ccb_t *pCCB = (tsm_ccb_t *)ccb;
	pCCB->out = out;
}
fmBytes *ccb_get_out_data(void *CCB)
{
    tsm_ccb_t *pCCB = (tsm_ccb_t *)CCB;
	pkg_entry_t *pe = pCCB->out;
	return pe->data;
}
byte_code_t *ccb_get_bytecode(void *ccb)
{
    tsm_ccb_t *pCCB = (tsm_ccb_t *)ccb;
	return pCCB->bc;
}
int ccb_get_session(void *ccb)
{
    tsm_ccb_t *pCCB = (tsm_ccb_t *)ccb;
	return pCCB->session;
}
void ccb_bre_entry_value_del(fmBytes *v)
{
	fmBytes_free(v);
}


zc_hashtable_t *ccb_get_bre(void *ccb)
{
    tsm_ccb_t *pCCB = (tsm_ccb_t *)ccb;
	return pCCB->bre;
}

void ccb_set_dest(void *ccb,int dest)
{
    tsm_ccb_t *pCCB = (tsm_ccb_t *)ccb;
	pCCB->out->agent = dest;
}
fmBytes_array_t *ccb_get_apdus(void *ccb)
{
    tsm_ccb_t *pCCB = (tsm_ccb_t *)ccb;
	return pCCB->apdus;
}
void ccb_set_apdus(void *ccb,fmBytes_array_t *apdus)
{
    tsm_ccb_t *pCCB = (tsm_ccb_t *)ccb;
	pCCB->apdus= apdus;
}
fmBytes *data_get_by_name(void *CCB,char *name)
{
    zc_hashtable_t *bre = ccb_get_bre(CCB);
	return (fmBytes *)zc_hashtable_get(bre,name);
}