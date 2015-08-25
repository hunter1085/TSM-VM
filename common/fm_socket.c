
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <errno.h>

#include "fm_Log.h"
#include "fm_socket.h"


int nSendBuf=36*1024;

int skt_server_init(int port)
{
    int server;
	struct sockaddr_in address;
	
    server=socket(AF_INET,SOCK_STREAM,0);
	memset(&address,0,sizeof(address));
	address.sin_family=AF_INET;
	address.sin_addr.s_addr=htonl(INADDR_ANY);
	address.sin_port = htons(port); 

	set_socket_reuseaddr(server);

	if(bind(server,(struct sockaddr *)(&address),sizeof(struct sockaddr)) != 0)  
	{
	    FM_LOGE("Bind failed! err=%s",strerror(errno));
		return -1;
	}

	if(listen(server, 16) != 0){
		FM_LOGE("Listen failed!");
		return -1;
	}

	set_nonblocking(server);
	return server;
}

int skt_server_accept(int server)
{
    int connfd;
	struct sockaddr_in clientaddr;
	socklen_t clilen;
	clilen = sizeof(clientaddr);
    connfd = accept(server,(struct sockaddr *)&clientaddr, &clilen);
	return connfd;
}


int set_socket_reuseaddr(int skt)
{
    int ret,on=1;

	ret = setsockopt(skt,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
	if(ret < 0){
		FM_LOGE("set socket opt failed!");
	}
	return ret;
}

void skt_do_accept(epoll_monitor_t *monitor,int server)
{
    int chd_fd;
    chd_fd = skt_server_accept(server);
	set_nonblocking(chd_fd);
	setsockopt(chd_fd,SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,sizeof(int));
	monitor_add_event(monitor,chd_fd,EPOLLIN|EPOLLET);
}
