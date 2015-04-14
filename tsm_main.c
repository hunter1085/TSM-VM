#include <pthread.h> 
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include "./common/fm_Log.h"
#include "./common/fm_timer.h"
#include "./common/fm_Mem.h"
#include "tsm_Err.h"
#include "tsm_router.h"
#include "tsm_cfg.h"
#include "tsm_ccb.h"
#include "tsm_vm.h"



#define SIGSUSPEND SIGUSR1
#define SIGRESUME SIGUSR2 

#define PROBE_CMD_SUSPEND_VM     0
#define PROBE_CMD_RESUME_VM      1



tsm_dev_t *tsm_dev;
static __thread int g_bSuspend;  


void suspend_handler(int signum)  
{  
    g_bSuspend = 1;  
      
    sigset_t nset;  
    pthread_sigmask(0, NULL, &nset);  
    /* make sure that the resume is not blocked*/  
    sigdelset(&nset, SIGRESUME);  
    while(g_bSuspend) sigsuspend(&nset);  
}  
  
void resume_handler(int signum)  
{  
    g_bSuspend = 0;  
}  
  
int suspend( pthread_t thread)  
{  
    return pthread_kill( thread, SIGSUSPEND);  
}  
  
int resume( pthread_t thread)  
{  
    return pthread_kill (thread, SIGRESUME);  
}  

void tsm_probe_loop()
{
    int i,cmd;
    pkg_entry_t  *pkg;
	while(1){
		sem_wait(&tsm_dev->sem_probe);
		pkg = list_dequeue_head(&tsm_dev->g_router->ppkg_list);
		cmd = atoi(pkg->data->buf);
		switch(cmd){
			case PROBE_CMD_SUSPEND_VM:
				//???should this time timer stops?
				for(i = 0; i < TSM_THREAD_NUM; i++){
					suspend(tsm_dev->threads[i]);
				}
				break;
			case PROBE_CMD_RESUME_VM:
				for(i = 0; i < TSM_THREAD_NUM; i++){
					resume(tsm_dev->threads[i]);
				}
				break;
		}
	}
}

int main(int argc,char *argv[])
{
    
#if 1
    int i,ret;
    struct sigaction suspendsa = {0};  
    struct sigaction resumesa = {0}; 

	tsm_dev = (tsm_dev_t *)fm_calloc(1,sizeof(tsm_dev_t));
	if(!tsm_dev){
		FM_LOGE("No memory");
		return -1;
	}

	/*the following signals are for tsm_probe thread*/
    suspendsa.sa_handler =  suspend_handler;  
    sigaddset(&suspendsa.sa_mask, SIGRESUME);  
    sigaction( SIGSUSPEND, &suspendsa, NULL);  
      
    resumesa.sa_handler = resume_handler;  
    sigaddset(&resumesa.sa_mask, SIGSUSPEND);  
    sigaction( SIGRESUME, &resumesa, NULL);

	/*init begins here*/
    fmLog_init("./zlog.conf");
	fmtimer_init(GR_MILLI_SECOND,2);
	tsm_dev->g_cfg = cfg_load("./tsm.conf");
	if(!tsm_dev->g_cfg){
		FM_LOGE("Load configure file failed!");
		return -1;
	}
	ret = router_init(tsm_dev->g_cfg,&tsm_dev->g_router);
	if(ret != FM_OK){
		FM_LOGE("Create router failed!");
		return -1;
	}

	list_init(&tsm_dev->ccb_list);

	ret = pthread_create(&tsm_dev->thrd_router_r, 0, router_aggregate_work, (void *)tsm_dev);
	if(ret != 0){
		FM_LOGE("Create Router read failed!");
		return -1;
	}
	ret = pthread_create(&tsm_dev->thrd_router_w, 0, router_disaggregate_work, (void *)tsm_dev);
	if(ret != 0){
		FM_LOGE("Create Router write failed!");
		return -1;
	}

	for(i = 0; i < TSM_THREAD_NUM; i ++){
		ret = pthread_create(&tsm_dev->threads[i], 0, tsm_vm, (void *)tsm_dev);
		if(ret != 0){
			FM_LOGE("Create TSM-VM thread pool failed!");
			return -1;
		}
	}
	tsm_probe_loop();
#endif
    return 0;
}
