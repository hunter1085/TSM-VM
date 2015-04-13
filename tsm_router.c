#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include "./common/fm_Base.h"
#include "./common/fm_Bytes.h"
#include "./common/fm_List.h"
#include "./common/fm_Log.h"
#include "./common/fm_Monitor.h"
#include "./common/fm_Log.h"
#include "./common/fm_Util.h"
#include "./common/fm_Mem.h"
#include "tsm_Err.h"
#include "tsm_ccb.h"
#include "tsm_bc.h"
#include "tsm_vm.h"
#include "tsm_router.h"

typedef struct{
	struct list_head entry;
	int agent_id;
	char *name;
	int rfd;
	int wfd;
}agent_entry_t;


LOCAL int router_get_des_wfd(int agent,list_t *agent_list)
{
    agent_entry_t *ae;
	struct list_head *pos,*n;

    LOCK(agent_list->lock);
	list_for_each_safe(pos,n,&agent_list->head){
		ae = (agent_entry_t *)pos;
		if(ae->agent_id == agent){
			LOCK(agent_list->lock);
			return ae->wfd;
		}
	}
	UNLOCK(agent_list->lock);
	return -1;
}
LOCAL int router_send(int fd,fmBytes *data)
{
    int ret,rest,pos;
	u8 *buf = fmBytes_get_buf(data);
    if(fd < 0){
		return -1;
    }

    rest = fmBytes_get_length(data);
	pos = 0;
	while(rest > 0){
	    ret = write(fd,buf+pos,rest);
		FM_LOGD("fd=0x%x,ret=0x%x,errno=0x%x,err=%s",fd,ret,errno,strerror(errno));
		if(ret > 0){
		    rest -= ret;
			pos += ret;
		}else{
		    if(errno != 0){/*bad file descriptor*/
				break;
		    }
		    usleep(10);
		}
		
	}
	return pos;
}

/*len:agent:session:ctl:data <==> 4:4:4:4:n*/
LOCAL pkg_entry_t *router_parse(int fd,tsm_router_t *router)
{
    int i,nbytes,len,field[4];
	u8 buf[4];
	struct list_head *pos,*n;
	agent_entry_t *ae;
	pkg_entry_t *pe;
	fmBool found;

	for(i = 0; i < 4; i++){
		nbytes = read(fd,buf,4);
		if(nbytes != 4){
			if((nbytes == 0)&&(i == 0)){
				FM_LOGD("The peer side closed");
			}else{
                clear_buffer(fd);
    		    FM_LOGE("socket read failed,fd=0x%x",fd);
			}
			return NULL;
		}
		field[i] = atoi(buf);
	}
	len = field[0]-12;
	
	pe = (pkg_entry_t *)fm_calloc(1,sizeof(pkg_entry_t));
	if(!pe) return NULL;

	pe->agent   = field[1];
	pe->session = field[2];
	pe->msg_type= field[3];
	pe->reason  = PKG_REASON_NORMAL;
	pe->data = fmBytes_alloc(len);
	
	if(!pe->data) return NULL;
	nbytes = read(fd,fmBytes_get_buf(pe->data),len);
	fmBytes_set_length(pe->data,len);

	/*search in the list*/
	found = fm_false;
	LOCK(router->agent_list.lock);
    list_for_each_safe(pos,n,&router->agent_list.head){
        ae = (agent_entry_t *)pos;
		if(ae->agent_id == field[1]){
			found = fm_true;
			break;
		}
	}
	UNLOCK(router->agent_list.lock);

	if(!found){
		fmBytes_free(pe->data);
		fm_free(pe);
		return NULL;
	}
	
    return pe;
	
	
}

LOCAL int router_ctrl_msg_handling(pkg_entry_t *pe,tsm_dev_t *tsm_dev)
{
	u8 ins;
	u8 *start,*buf;
	char *script;
	int session;
	tsm_ccb_t *CCB;
	dl_array_t *dls;

	if((!pe)||(!pe->data)) return FM_ROUTER_INVALID_PARM;
	buf = fmBytes_get_buf(pe->data);
	ins = buf[0];
	start = buf[1];session = atoi(start);
	switch(ins){
		case 0:
			
		    CCB = ccb_create_ccb(&tsm_dev->bc_list,session,dls,script);
			list_add_entry(&tsm_dev->ccb_list,&CCB->entry);
			break;
		default:
			break;
	}
	return 0;
}

PUBLIC void *router_aggregate_work(void* para)
{
    tsm_dev_t *tsm_dev = (tsm_dev_t *)para;
    tsm_router_t *router = tsm_dev->g_router;
    int i,nfds;
	epoll_monitor_t *monitor = &(router->monitor);
	pkg_entry_t *pe;

    while(1){
    	nfds = monitor_wait_event(monitor,-1);
    	for(i=0;i<nfds;++i){
            if(monitor_event_at(monitor,i) & EPOLLIN){/*get data,data=EOF indicate the terminal closed*/
    		    pe = router_parse(monitor->events[i].data.fd,router);
    			if(!pe){
        			if(pe->agent == AG_TOOL){
    					list_queue_tail(&router->ppkg_list,pe->entry);
        				sem_post(&tsm_dev->sem_probe);
        			}else{/*msgs from both B-Agent and F-Agent are sent direct to VM*/
        			    if(pe->msg_type == MSG_CTRL){
                            router_ctrl_msg_handling(pe,tsm_dev);
        			    }
        			    list_queue_tail(&router->rpkg_list,pe->entry);
        			    sem_post(&tsm_dev->sem_vm);
        			}
    				
    			}
    		}else{
    		}
    	}
    }
	return (void *)0;
}

PUBLIC void *router_disaggregate_work(void* para)
{
    tsm_dev_t *tsm_dev = (tsm_dev_t *)para;
    tsm_router_t *router = tsm_dev->g_router;
	pkg_entry_t *pe;
	fmBytes *pkg;
	u8 *buf;
	int pos,len,fd;
    while(1){
		sem_wait(&tsm_dev->sem_router_w);
		pe = (pkg_entry_t *)list_dequeue_head(&router->ppkg_list);
		len = fmBytes_get_length(pe->data)+12;
		pkg = fmBytes_alloc(len);
		buf = fmBytes_get_buf(pkg);
		pos = 0;
		pos += int2byteBE(len,buf+pos);
		pos += int2byteBE(pe->agent,buf+pos);
		pos += int2byteBE(pe->session,buf+pos);
		pos += int2byteBE(MSG_DATA,buf+pos);
		memcpy(buf+pos,fmBytes_get_buf(pe->data),len-12);
		pos += len-12;
		fmBytes_set_length(pkg,pos);
		fd = router_get_des_wfd(pe->agent,&router->agent_list);
		if(fd != -1)router_send(fd,pkg);
    }
	return (void *)0;
}

PUBLIC int router_init(tsm_cfg_t *cfg,tsm_router_t **rt)
{
    int res;
	agent_entry_t *ae;
	struct list_head *pos,*n;
	agent_info_t *agent;
    tsm_router_t *router;
	
	router = (tsm_router_t *)fm_calloc(1,sizeof(tsm_router_t));
	if(!router) return FM_SYS_ERR_NO_MEM;
    monitor_init(&(router->monitor));
	list_init(&router->agent_list);
	list_init(&router->rpkg_list);
	list_init(&router->wpkg_list);
	list_init(&router->ppkg_list);

    LOCK(cfg->ag_list.lock);
	list_for_each_safe(pos,n,&cfg->ag_list.head){
		agent = (agent_info_t *)pos;
		if(access(agent->agent_r_fifo, F_OK) == -1){
			res = mkfifo(agent->agent_r_fifo,0777);
			if(res != 0){
				FM_LOGE("Create agent read fifo failed!");
				exit(EXIT_FAILURE);
			}
		}
		if(access(agent->agent_w_fifo, F_OK) == -1){
			res = mkfifo(agent->agent_w_fifo,0777);
			if(res != 0){
				FM_LOGE("Create agent write fifo failed!");
				exit(EXIT_FAILURE);
			}
		}	

        /*agent write side is router read side*/
		ae = (agent_entry_t *)fm_calloc(1,sizeof(agent_entry_t));
		ae->agent_id = agent->agent_id;
		ae->name = fm_calloc(strlen(agent->name)+1,sizeof(char));
		memcpy(ae->name,agent->name,strlen(agent->name));
		ae->rfd = open(agent->agent_w_fifo,O_RDONLY);
		ae->wfd = open(agent->agent_r_fifo,O_WRONLY);
		

		/*add the router read fd to the monitor*/
		monitor_add_event(&router->monitor,ae->rfd,EPOLLIN|EPOLLET);

		list_add_entry(&router->agent_list,ae->entry);
	}
	UNLOCK(cfg->ag_list.lock);
	*rt = router;
	return FM_OK;
}

