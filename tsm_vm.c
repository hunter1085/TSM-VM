#include "./common/fm_Base.h"
#include "./common/fm_List.h"
#include "tsm_ccb.h"
#include "tsm_router.h"
#include "tsm_scloader.h"
#include "tsm_vm.h"

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
LOCAL code_entry_t *fetch_code_entry(int pc,script_entry_t *se) 
{
    struct list_head *pos,*n;
	code_entry_t *ce;
    list_t *c_list = &se->c_list;

    LOCK(c_list->lock);
	list_for_each_safe(pos,n,&c_list->head){
		ce = (code_entry_t *)pos;
		if(pc == ce->addr){
			UNLOCK(c_list->lock);
			return ce;
		}
	}
	UNLOCK(c_list->lock);
	return NULL;
}


PUBLIC void *tsm_vm(void* para)
{
    struct list_head *entry;
	pkg_entry_t  *pkg,*out;
	code_entry_t *ce;
	byte_code_t *bc_array;
	void *CCB;
	tsm_dev_t *tsm_dev = (tsm_dev_t *)para;
	tsm_router_t *router = tsm_dev->g_router;
	list_t *ccb_list = &tsm_dev->ccb_list;
	
	while(1){
        sem_wait(&tsm_dev->sem_vm);
    	entry = list_dequeue_head(&router->rpkg_list);
    	pkg = (pkg_entry_t *)entry;
		CCB = fetch_ccb(ccb_list,pkg->session);
		if(CCB == NULL){
		}
		ce = fetch_code_entry(ccb_get_PC(CCB),ccb_get_script(CCB));
		bc_array = ccb_get_bytecode(CCB);
		bc_array->bc[ce->code](CCB,pkg->data,&ce->parameters);
		if(ccb_get_write_back(CCB)){
			ccb_set_write_back(CCB,fm_false);
			out = ccb_get_out_pkg(CCB);
			list_queue_tail(&router->wpkg_list,&out->entry);
			sem_post(&tsm_dev->sem_router_w);
		}
		if(ccb_get_bootstrap(CCB)){
			ccb_set_bootstrap(CCB,fm_false);
			pkg->reason = PKG_REASON_BOOTSTRAP;
			list_queue_tail(&router->rpkg_list,&pkg->entry);
			sem_post(&tsm_dev->sem_vm);
		}
		
	}
	return (void *)0;
}
