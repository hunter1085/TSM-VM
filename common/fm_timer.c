#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include "fm_Log.h"
#include "fm_timer.h"

typedef struct _fmtimer_node_{
    struct list_head entry;
	int expires;
	void (*function)(int); 
	int data;
	int timerID;
}fmtimer_node;

typedef struct _fmtimer_list_{
	struct list_head active_list;
	struct list_head expire_list;
	int active_cnt;
	int expire_cnt;
	lock_t lock;
	
	timer_t tm;
	struct sigevent se;
	struct itimerspec its;
	int interval;   /*the interval = its->tv_sec,it's for use convinience*/

	int timer_thrd;
	sem_t sem_timer;
}fmtimer_list;

static fmtimer_list tl;
static int timerID = 0;

LOCAL void clocktick_handler(union sigval sv)
{
    fmtimer_node *node;
	struct list_head *pos,*n;

    LOCK(tl.lock);    
	list_for_each_safe(pos,n,&tl.active_list){
		/*as "entry" addr is same as the structure,
		we explicitly convert it as the node addr*/
		node = (fmtimer_node *)pos; 
        node->expires -= tl.interval;
		if(node->expires <= 0){
			list_del(&node->entry);
			tl.active_cnt--;
			list_add_tail(&node->entry,&tl.expire_list);
			tl.expire_cnt++;
		}
	}
	UNLOCK(tl.lock);
	
	if(tl.expire_cnt > 0) sem_post(&tl.sem_timer);
}

LOCAL void *to_handler(void *arg)
{
    fmtimer_node *node;
	struct list_head *pos,*n;

	
    sem_wait(&tl.sem_timer);

	//FM_LOGD("get timer sem");
	
    LOCK(tl.lock);    
	list_for_each_safe(pos,n,&tl.expire_list){
		node = (fmtimer_node *)pos; 
		list_del(&node->entry);
        if(node->function != NULL){
			node->function(node->data);
        }
		fm_free(node);
	}
	UNLOCK(tl.lock);
}

/*create an periodic timer,period=gr*interval*/
PUBLIC int fmtimer_init(timer_granularity gr,int interval)
{
    int ns=0,s=0;

    switch(gr){
		case GR_MILLI_SECOND:
			s = interval/1000;
			ns = (interval%1000)*1000;
			break;
		case GR_SECOND:
			s = interval;
			ns = 0;
			break;
		case GR_MINUTE:
			s = interval*60;
			ns = 0;
			break;
		case GR_HOUR:
			s = interval*360;
			ns = 0;
			break;
		default:
			s = ns = 0;
			break;
    }
    memset(&tl,0,sizeof(tl));
    LOCK_INIT(tl.lock);
	INIT_LIST_HEAD(&tl.active_list);
	INIT_LIST_HEAD(&tl.expire_list);
	tl.active_cnt = 0;
	tl.expire_cnt = 0;
	sem_init(&tl.sem_timer,0,0);

	tl.its.it_interval.tv_sec = s;
    tl.its.it_interval.tv_nsec= ns;
    tl.its.it_value.tv_sec    = 0;
    tl.its.it_value.tv_nsec   = 1000;   /*the first time peroid*/
    tl.se.sigev_notify = SIGEV_THREAD;
    tl.se.sigev_notify_function = clocktick_handler;
	tl.se.sigev_notify_attributes = NULL;
	tl.interval = interval;

    if(pthread_create(&tl.timer_thrd,NULL,to_handler,NULL) != 0){
		FM_LOGE("create timer thread failed!");
        return -1;
    }

    if(timer_create(CLOCK_REALTIME,&(tl.se),&(tl.tm)) < 0)
    {
    	FM_LOGE("create clocktick timer failed!");
        return -1;
    }

	timer_settime(tl.tm,0,&tl.its,NULL);
	return 0;
}

/*return the timerID*/
PUBLIC int fmtimer_add(u32 to,void (*function)(int),int data)
{
    fmtimer_node *node;

	node = (fmtimer_node *)fm_malloc(sizeof(fmtimer_node));
	node->expires = to;
	node->function = function;
	node->data = data;
	node->timerID = timerID++;

	//FM_LOGD("timer node=%p",node);

    LOCK(tl.lock);    
	list_add_tail(&node->entry,&tl.active_list);
	tl.active_cnt++;
	UNLOCK(tl.lock);

	return node->timerID;
}

PUBLIC int fmtimer_rearm(int timerID,int new_expire)
{
    fmtimer_node *node;
	struct list_head *pos,*n;

    LOCK(tl.lock);    
	list_for_each_safe(pos,n,&tl.active_list){
		node = (fmtimer_node *)pos;
    	if(node->timerID == timerID){
			node->expires = new_expire;
			UNLOCK(tl.lock);
			return 0;
    	}
	}
	UNLOCK(tl.lock);
	return -1;
}

PUBLIC void fmtimer_del(int timerID)
{
    fmtimer_node *node;
	struct list_head *pos,*n;

    LOCK(tl.lock);    
	list_for_each_safe(pos,n,&tl.active_list){
		node = (fmtimer_node *)pos;
    	if(node->timerID == timerID){
    		list_del(&node->entry);
			tl.active_cnt--;
			fm_free(node);
    	}
	}
	UNLOCK(tl.lock);
    
}
PUBLIC void fmtimer_deinit()
{
    fmtimer_node *node;
	struct list_head *pos,*n;
	
	LOCK(tl.lock);
    list_for_each_safe(pos,n,&tl.active_list){
        list_del(&node->entry);
        node = (fmtimer_node *)pos;
		fm_free(node);
    }
	UNLOCK(tl.lock);

	LOCK_DESTROY(tl.lock);
}


