#ifndef _FMLOG_H_
#define _FMLOG_H_
#include "zlog.h"
extern zlog_category_t * log_category;
int fmLog_init(char *cfg_file);
void fmLog_fini();

#define FM_LOGI(format, args...)     \
	zlog(log_category, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	ZLOG_LEVEL_INFO, format, ##args)

#define FM_LOGD(format, args...)     \
	zlog(log_category, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	ZLOG_LEVEL_DEBUG, format, ##args)

#define FM_LOGE(format, args...)     \
	zlog(log_category, __FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	ZLOG_LEVEL_ERROR, format, ##args)
	
#endif