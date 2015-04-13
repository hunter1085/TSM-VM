#define AG_MOBILE_PORT  0x8444
#define AG_MOBILE_THRDS 5
int nSendBuf=36*1024;
typedef struct{
    int server;
	struct sk_buff_head rq_terminal;
	struct sk_buff_head rq_router;
	sem_t sem_terminal;
	sem_t sem_router;
	epoll_monitor_t monitor;

    char *fifo_name_r;
	char *fifo_name_w;
	int fifo_r;
	int fifo_w;
}ag_mobile_t;
ag_mobile_t *agent_m;
void on_sigint(int signal)
{
    FM_LOGD("sigint");
	close_socket(mobile_dev->server);
    exit(0);
}

int fifo_init(ag_mobile_t * agent)
{
    int res;
	if(access(agent->fifo_name_r, F_OK) == -1){
		res = mkfifo(agent.fifo_r,0777);
		if(res != 0){
			FM_LOGE("Create agent read fifo failed!");
			return res;
		}
	}
	if(access(agent.fifo_name_w, F_OK) == -1){
		res = mkfifo(agent.fifo_w,0777);
		if(res != 0){
			FM_LOGE("Create agent write fifo failed!");
			return res;
		}
	}	
	set_nonblocking(agent.fifo_r);
	set_nonblocking(agent.fifo_w);
	monitor_add_event(&agent->monitor,agent.fifo_r,EPOLLIN|EPOLLET);
	monitor_add_event(&agent->monitor,agent.fifo_w,EPOLLOUT|EPOLLET);
	return FM_OK;
}
void do_accept(epoll_monitor_t *monitor,int server)
{
    int chd_fd;
    chd_fd = ag_server_accept(server);
	set_nonblocking(chd_fd);
	setsockopt(chd_fd,SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,sizeof(int));
	monitor_add_event(monitor,chd_fd,EPOLLIN|EPOLLET);
}
sk_buff *do_read(epoll_monitor_t *monitor,int fd)
{
     int nread,len;
	 u8 len_buf[4];
	 struct sk_buff *skb = NULL;
	 	
     nread = read(fd,len_buf,4);
     if (nread == -1)
     {
         FM_LOGE("read error:");
     }
     else if (nread == 0)
     {
         FM_LOGE("client close.");
     }
     else
     {
         if(len_buf[0] == (~len_buf[1])){
    		 len = (((len_buf[2]&0xff)<<8) | (len_buf[3]&0xFF));
    		 skb = alloc_skb(len);
    		 nread = read(fd,(skb_put(skb,len)),len);
    		 return skb;
         }

         //修改描述符对应的事件，由读改为写
         //modify_event(epollfd,fd,EPOLLOUT);
     }
	 
     close(fd);
     monitor_del_event(monitor,fd,EPOLLIN);
	 return skb;
}
int main(int argc,char *argv[])
{
    int i,nfds,fd;
    ag_mobile_t *pAgent;
	struct sk_buff *skb;
	
    signal(SIGINT,&on_sigint);
	
    pAgent = fm_calloc(1,sizeof(ag_mobile_t));
	if(!pAgent) return -1;
	agent_m = pAgent;

	fmLog_init("./zlog.conf");
	
    sem_init(&pAgent->sem_terminal,0,0);
	sem_init(&pAgent->sem_router,0,0);
	skb_queue_head_init(&pAgent->rq_terminal);
	skb_queue_head_init(&pAgent->rq_router);

	monitor_init(&pAgent->monitor);
	pAgent->server = ag_server_init(AG_MOBILE_PORT);
	monitor_add_event(&pAgent->monitor,pAgent->server,EPOLLIN);

	fifo_init(pAgent);

	fmtimer_init(GR_SECOND,10);
	init_thread_pool(AG_MOBILE_THRDS,terminal_handling_work);
	init_thread_pool(AG_MOBILE_THRDS,router_handling_work);
	while(1){
		nfds = monitor_wait_event(&pAgent->monitor,-1);
		for(i=0;i<nfds;++i){
			if(monitor_fd_at(&pAgent->monitor,i) == pAgent->server){
				do_accept(&pAgent->monitor,pAgent->server);
			}else if(monitor_event_at(&pAgent->monitor,i)&EPOLLIN){
			    fd = monitor_fd_at(&pAgent->monitor,i);
				skb = do_read(&pAgent->monitor,fd);
				if(!skb){
					if(fd == pAgent->fifo_r){
						skb_queue_tail(&pAgent->rq_router,skb);
						sem_post(&pAgent->sem_router);
					}else{
						skb_queue_tail(&pAgent->rq_terminal,skb);
						sem_post(&pAgent->sem_terminal);
					}
				}
			}else{
			}
		}
	}
    return 0;
}
