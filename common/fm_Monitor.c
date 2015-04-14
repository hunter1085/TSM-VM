#include <string.h>
#include "fm_Monitor.h"
int monitor_init(epoll_monitor_t *monitor)
{
	monitor->epfd = epoll_create(65535);
	return 0;
}

int monitor_add_event(epoll_monitor_t *monitor,int fd,int ev)
{
    struct epoll_event event;

    memset(&event,0,sizeof(event));
    event.data.fd=fd;
    event.events=ev;
	epoll_ctl(monitor->epfd,EPOLL_CTL_ADD,fd,&event);
	return 0;
}
int monitor_del_event(epoll_monitor_t *monitor,int fd,int ev)
{
    struct epoll_event event;

    memset(&event,0,sizeof(event));
    event.data.fd=fd;
    event.events=ev;
	epoll_ctl(monitor->epfd,EPOLL_CTL_DEL,fd,&event);
	return 0;
}
int monitor_modify_event(epoll_monitor_t *monitor,int fd,int ev)
{
    struct epoll_event event;

    memset(&event,0,sizeof(event));
    event.data.fd=fd;
    event.events=ev;
	epoll_ctl(monitor->epfd,EPOLL_CTL_MOD,fd,&event);
	return 0;
}
int monitor_wait_event(epoll_monitor_t *monitor,int to)
{
    return epoll_wait(monitor->epfd,monitor->events,FMSH_TSM_MAX_EVENTS,to);
}
int monitor_fd_at(epoll_monitor_t *monitor,int index)
{
    return monitor->events[index].data.fd;
}
int monitor_event_at(epoll_monitor_t *monitor,int index)
{
    return monitor->events[index].events;
}