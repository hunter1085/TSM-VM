#ifndef _FM_TIMER_H_
#define _FM_TIMER_H_

#include "fm_Base.h"

typedef enum _timer_granularity_{
    GR_MILLI_SECOND = 0x01,
	GR_SECOND       = 0x02,
	GR_MINUTE       = 0x03,
	GR_HOUR         = 0x04,
}timer_granularity;

PUBLIC int fmtimer_init(timer_granularity gr,int interval);
PUBLIC int fmtimer_add(u32 to,void (*function)(int),int data);
PUBLIC int fmtimer_rearm(int timerID,int new_expire);
PUBLIC void fmtimer_del(int timerID);
PUBLIC void fmtimer_deinit();
#endif
