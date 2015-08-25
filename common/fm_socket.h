#ifndef _FM_SOCKET_H_
#define _FM_SOCKET_H_
#include "fm_Monitor.h"
int skt_server_init(int port);
int skt_server_accept(int server);
int set_socket_reuseaddr(int skt);
void skt_do_accept(epoll_monitor_t *monitor,int server);
#endif
