#include <semaphore.h>
#include "fm_Base.h"
#include "fm_Log.h"

zlog_category_t * log_category = NULL;

int fmLog_init(char *cfg_file)
{
    int i;
    if(zlog_init(cfg_file)){
		printf("Error: zlog_init\n");
        zlog_fini();
        return -1;
    }
	
    log_category = zlog_get_category("tsm");
    if (!log_category) {
        printf("Error: get cat fail\n");
        zlog_fini();
        return -2;
    }
	
}



void fmLog_fini() 
{
    zlog_fini();
}