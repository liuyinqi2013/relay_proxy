#ifndef     __RELAY_LOG_H__
#define     __RELAY_LOG_H__

#include    "log.h"
#include    "system_conf.h"

void init_log(const char * log_path, int log_size, int log_num);

void fini_log();

#endif

