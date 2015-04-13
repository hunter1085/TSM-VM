#ifndef _TSM_VM_H_
#define _TSM_VM_H_

#include <pthread.h> 
#include <semaphore.h>
#include "./common/fm_List.h"
#include "tsm_cfg.h"
#include "tsm_router.h"

#define TSM_THREAD_NUM  5
typedef struct {
    pthread_t thrd_router_r;
    pthread_t thrd_router_w;
    pthread_t threads[TSM_THREAD_NUM];//thread pool for tsm-vm
	tsm_cfg_t *g_cfg;
	tsm_router_t *g_router;
	

	sem_t sem_probe;
	sem_t sem_vm;
	sem_t sem_router_w;

	list_t ccb_list;
	list_t bc_list;
}tsm_dev_t;

PUBLIC int tsm_reserve();
PUBLIC void *tsm_vm(void* para);

#endif
