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
	char *name;
	int fd;
}agent_entry_t;


LOCAL agent_entry_t *router_register_agent(int fd,list_t *tlv)
{
    struct list_head *pos,*n;
	router_tlv_t *node;
	agent_entry_t *ae;

    ae = (agent_entry_t *)fm_calloc(1,sizeof(agent_entry_t));
	if(!ae) return NULL;
	ae->fd = fd;
	list_for_each_safe(pos,n,&tlv->head){
		node = (router_tlv_t *)pos;
		if(node->tag == TAG_AG_NAME){
			ae->name = (u8 *)fm_calloc(node->len+1,sizeof(char));
			memcpy(ae->name,node->val,node->len);
			break;
		}
	}
	return ae;
}

LOCAL int router_get_des_wfd(u8 *name,list_t *agent_list)
{
    agent_entry_t *ae;
	struct list_head *pos,*n;

    LOCK(agent_list->lock);
	list_for_each_safe(pos,n,&agent_list->head){
		ae = (agent_entry_t *)pos;
		if(strcmp(name,ae->name) == 0){
			LOCK(agent_list->lock);
			return ae->fd;
		}
	}
	UNLOCK(agent_list->lock);
	return -1;
}
LOCAL int router_send(int fd,u8 *buf,int len)
{
    int ret,rest,pos;
    if(fd < 0){
		return -1;
    }

    rest = len;
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
/*len:agent:session:ctl:data <==> 4:4:4:4:n*/
struct sk_buff *router_do_read(int fd,int len)
{
    int nread,left,i,field[2];
	u8 *pos;
	u8 buf[4];
	struct sk_buff *skb;

	for(i = 0; i < 2; i++){
		nread = read(fd,buf[i],4);
		if(nread != 4){
			clear_buffer(fd);
			FM_LOGE("socket read failed,fd=0x%x",fd);
			return NULL;
		}
		field[i] = atoi(buf);
	}
	skb = alloc_skb(len);
	pos = skb_put(skb,len);
	skb->msg_type= field[0];
	skb->session = field[1];
	skb->reason  = PKG_REASON_NORMAL;
	skb->isNew   = fm_true;

    left = len-12;
	while(left>0){
		nread = read(fd,pos,left);
		left -= nread;
		pos  += nread;
	}
	return skb;
}

void router_unregister_agent(int fd,epoll_monitor_t *monitor,list_t *ag_list)
{
    struct list_head *pos,*n;
    agent_entry_t *ae;
	
    close(fd);
    monitor_del_event(monitor,fd,NULL);
	LOCK(ag_list->lock);
    list_for_each_safe(pos,n,&ag_list->head){
    	ae = (agent_entry_t *)pos;
		if(ae->fd == fd){
			list_del_entry(ag_list,&ae->entry);
    		UNLOCK(ag_list->lock);
			fm_free(ae->name);
			fm_free(ae);
			return;
		}
    }
	UNLOCK(ag_list->lock);
    
}
PUBLIC void *router_aggregate_work(void* para)
{
    tsm_dev_t *tsm_dev = (tsm_dev_t *)para;
    tsm_router_t *router = tsm_dev->g_router;
    int i,nfds,fd,nread,len;
	u8 len_buf[4];
	epoll_monitor_t *monitor = &(router->monitor);
	struct sk_buff *skb = NULL;
	while(1){
		nfds = monitor_wait_event(monitor,-1);
		for(i=0;i<nfds;++i){
			if(monitor_fd_at(&router->monitor,i) == router->server){
				skt_do_accept(&router->monitor,router->server);
			}else if(monitor_event_at(&router->monitor,i)&EPOLLIN){
			    fd = monitor_fd_at(&router->monitor,i);
				nread = read(fd,len_buf,4);
				if (nread == -1){
					FM_LOGE("read error");
					continue;
				}else if(nread == 0){
				    FM_LOGI("client closed!");
					router_unregister_agent(fd,&router->monitor,&router->agent_list);
					continue;
				}

				len = atoi(len_buf);
				skb = router_do_read(fd,len);
				if(!skb)continue;

				if((skb->msg_type>>24) == MSG_CTRL){
                	u8 ins;
					list_t *tlv;

					ins = *skb_pull(skb,1);
					tlv = router_parse_tlv(skb->data,skb->len);
					skb_pull(skb->data,skb->len);
                	switch(ins){
                		case INS_CREATE_CCB://create ccb
                		{
                			tsm_ccb_t *CCB;
                		    CCB = ccb_create_ccb(&tsm_dev->bc_list,tlv);
							skb->session = ccb_get_session(CCB);
                			list_add_entry(&tsm_dev->ccb_list,&CCB->entry);
							skb_queue_tail(&router->rpkg_list,skb);
							sem_post(&tsm_dev->sem_vm);
                		}
                			break;
						case INS_REG_AGENT://register agent
						{
							u8 *buf;
							int name_len;
							agent_entry_t *ae;
							struct sk_buff *snd_skb = NULL;
							
						    ae = router_register_agent(fd,tlv);
							list_add_entry(&router->agent_list,ae->entry);
							kfree_skb(skb);

							/*ACK:RESULT*/
							name_len = strlen(ae->name);
							snd_skb = alloc_skb(2+MSG_HEAD_LEN);
							skb_reserve(snd_skb,MSG_HEAD_LEN);
							*skb_put(snd_skb,1) = ACK_REG_AGENT;
							*skb_put(snd_skb,1) = 0;
							snd_skb->msg_type = MSG_CTRL;
							snd_skb->agent = fmBytes_alloc(name_len+1);
							buf = fmBytes_get_buf(snd_skb->agent);
							memcpy(buf,ae->name,name_len);
							skb_queue_tail(&router->wpkg_list,snd_skb);
							sem_post(&tsm_dev->sem_router_w);
						}
							break;
						case INS_UNREG_AGENT://unregister agent
						{
						}
						    break;
                		default:
                			break;
                	}
				}else{
				    if(skb->agent == AG_TOOL){
				        skb_queue_tail(&router->ppkg_list,skb);
						sem_post(&tsm_dev->sem_probe);	
				    }else{
				        skb_queue_tail(&router->rpkg_list,skb);
						sem_post(&tsm_dev->sem_vm);
				    }
				}
				
			}else{
			}
		}
	}
	return (void *)0;
}

/********************************************************************************
A0 contains a single APDU,A1 contains various of A1.
A0 len(1/3) seq data 
A1 len(1/3) data 
********************************************************************************/
PUBLIC void *router_disaggregate_work(void* para)
{
    tsm_dev_t *tsm_dev = (tsm_dev_t *)para;
    tsm_router_t *router = tsm_dev->g_router;
	u8 buf[4];
	int pos,len,fd;
	struct sk_buff *skb;
    while(1){
		sem_wait(&tsm_dev->sem_router_w);

		/*packing MSG HEAD and sends*/
		skb = skb_dequeue(&router->wpkg_list);
		int2byteBE(skb->session,buf);
        memcpy(skb_push(skb,4),buf,4);
		int2byteBE(skb->msg_type,buf);
		memcpy(skb_push(skb,4),buf,4);
		int2byteBE(skb->len,buf);
		memcpy(skb_push(skb,4),buf,4);
		
		fd = router_get_des_wfd(skb->agent->buf,&router->agent_list);
		if(fd != -1)router_send(fd,skb->data,skb->len);
		kfree_skb(skb);
    }
	return (void *)0;
}

PUBLIC int router_init(tsm_cfg_t *cfg,tsm_router_t **rt)
{
    tsm_router_t *router;
	
	router = (tsm_router_t *)fm_calloc(1,sizeof(tsm_router_t));
	if(!router) return FM_SYS_ERR_NO_MEM;
    monitor_init(&(router->monitor));
	list_init(&router->agent_list);
	skb_queue_head_init(&router->rpkg_list);
	skb_queue_head_init(&router->wpkg_list);
	skb_queue_head_init(&router->ppkg_list);

	router->server = skt_server_init(cfg->port);
	if(router->server < 0){
		fm_free(router);
		return FM_ROUTER_CREATE_SVR_FAILED;
	}

	monitor_add_event(&router->monitor,router->server,EPOLLIN);
	*rt = router;
	return FM_OK;
}

