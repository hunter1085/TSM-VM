#ifndef _FM_MONITOR_H_
#define _FM_MONITOR_H_
#include <sys/epoll.h>
#define FMSH_TSM_MAX_EVENTS         1024    /*capability of handle concurrent events*/

typedef struct{
	int epfd;
	struct epoll_event events[FMSH_TSM_MAX_EVENTS];
}epoll_monitor_t;

int monitor_init(epoll_monitor_t *monitor);
int monitor_add_event(epoll_monitor_t *monitor,int fd,int ev);
int monitor_del_event(epoll_monitor_t *monitor,int fd,int ev);
int monitor_modify_event(epoll_monitor_t *monitor,int fd,int ev);
int monitor_wait_event(epoll_monitor_t *monitor,int to);
int monitor_fd_at(epoll_monitor_t *monitor,int index);
int monitor_event_at(epoll_monitor_t *monitor,int index);
#endif
