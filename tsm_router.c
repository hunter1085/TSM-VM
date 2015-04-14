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
#include "bc.h"
#include "tsm_Err.h"
#include "tsm_ccb.h"
#include "tsm_vm.h"
#include "tsm_router.h"

typedef struct{
	struct list_head entry;
	int agent_id;
	char *name;
	int rfd;
	int wfd;
}agent_entry_t;

LOCAL int router_create_fifo(char *fifo_name)
{
    int res;
	if(access(fifo_name, F_OK) == -1){
		res = mkfifo(fifo_name,0777);
		if(res != 0){
			FM_LOGE("Create agent read fifo failed!");
			return -1;
		}
	}
	return 0;
}

LOCAL agent_entry_t *router_register_agent(list_t *tlv)
{
    struct list_head *pos,*n;
	router_tlv_t *node;
	agent_entry_t *ae;

    ae = (agent_entry_t *)fm_calloc(1,sizeof(agent_entry_t));
	if(!ae) return NULL;
	list_for_each_safe(pos,n,&tlv->head){
		node = (router_tlv_t *)pos;
		if(node->tag == TAG_AG_ID){
			ae->agent_id = atoi(node->val);
			continue;
		}
		if(node->tag == TAG_AG_NAME){
			ae->name = (u8 *)fm_calloc(node->len+1,sizeof(char));
			memcpy(ae->name,node->val,node->len);
			continue;
		}
		/*agent write side is router read side*/
		if(node->tag == TAG_AG_R_FIFO){
			router_create_fifo(node->val);
			ae->wfd = open(node->val,O_WRONLY);
			continue;
		}
		if(node->tag == TAG_AG_W_FIFO){
			router_create_fifo(node->val);
			ae->rfd = open(node->val,O_RDONLY);
			continue;
		}
	}
	return ae;
}

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


/*
all tlv len is 2 byte
*/
LOCAL list_t *router_parse_tlv(u8 *str,int len)
{
    int i;
	list_t *tlv_list;
	router_tlv_t *tlv;

	tlv_list = (list_t *)fm_calloc(1,sizeof(list_t));
	if(!tlv_list) return NULL;
	list_init(tlv_list);

	for(i = 0; i < len;){
		tlv = (router_tlv_t *)fm_calloc(1,sizeof(router_tlv_t));
		tlv->tag = str[i];
		tlv->len = (str[i+1]<<8)|(str[i+2]);
		tlv->val = (u8 *)fm_calloc(tlv->len,sizeof(u8));
		memcpy(tlv->val,str+3,tlv->len);
		i += tlv->len;
		list_add_entry(tlv_list,&tlv->entry);
	}
	return tlv_list;
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
        			}else if(pe->msg_type == MSG_CTRL){
                        	u8 ins;
                        	u8 *buf;
							int len;
							
							list_t *tlv;
							
                        	buf = fmBytes_get_buf(pe->data);
							len = fmBytes_get_length(pe->data);
                        	ins = buf[0];
							tlv = router_parse_tlv(buf+1,len-1);
							
                        	switch(ins){
                        		case 0://create ccb
                        		{
                        			tsm_ccb_t *CCB;
                        		    CCB = ccb_create_ccb(&tsm_dev->bc_list,tlv);
                        			list_add_entry(&tsm_dev->ccb_list,&CCB->entry);
									list_queue_tail(&router->rpkg_list,pe->entry);
									sem_post(&tsm_dev->sem_vm);
                        		}
                        			break;
								case 1://register agent
								{
									agent_entry_t *ae;
								    ae = router_register_agent(tlv);
									monitor_add_event(&router->monitor,ae->rfd,EPOLLIN|EPOLLET);
									list_add_entry(&router->agent_list,ae->entry);
								}
									break;
                        		default:
                        			break;
                        	}
        			}else{
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
		router_create_fifo(agent->agent_r_fifo);
		router_create_fifo(agent->agent_w_fifo);

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

