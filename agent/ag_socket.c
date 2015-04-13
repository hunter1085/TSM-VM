int ag_server_init(int port)
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
		return FM_ROUTER_BIND_FAILED;
	}

	if(listen(server, 16) != 0){
		FM_LOGE("Listen failed!");
		return FM_ROUTER_LISTEN_FAILED;
	}

	set_nonblocking(server);
	return server;
}

int ag_server_accept(int server)
{
    int connfd;
	struct sockaddr_in clientaddr;
	socklen_t clilen;
	clilen = sizeof(clientaddr);
    connfd = accept(server,(struct sockaddr *)&clientaddr, &clilen);
	return connfd;
}

int ag_client_connet(char *ip,int port)
{
    int fd,ret;
	struct sockaddr_in address;
	
    fd = socket(AF_INET,SOCK_STREAM,0);
	memset(&address,0,sizeof(address));
	address.sin_family=AF_INET;
	address.sin_addr.s_addr=inet_addr(ip);
	address.sin_port = htons(port);
	
    ret = connect(fd,(struct sockaddr *)(&address),sizeof(struct sockaddr));
	if(ret != 0){
		FM_LOGD("Connect to Front-Agent failed!");
		return 0;
	}
	return fd;
}