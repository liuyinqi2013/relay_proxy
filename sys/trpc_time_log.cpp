#include <stdarg.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include "trpc_time_log.h"

/**
 * 每次记录的日志缓存
 */
#define CFT_LOG_STR(str)\
    int iBufLen = 0;\
    time_t tnow;\
    struct tm tm;\
    tnow = time(NULL);\
    localtime_r(&tnow, &tm);\
    iBufLen = snprintf(_log_buf, sizeof(_log_buf), \
    "|%04d-%02d-%02d %02d:%02d:%02d|%d|%d|%s|",\
            tm.tm_year + 1900, tm.tm_mon + 1,\
            tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,\
            _proc_no, __pid, str);\
\
    va_list ap;\
    va_start(ap, fmt);\
    vsnprintf(_log_buf + iBufLen, sizeof(_log_buf) - iBufLen, fmt, ap);\
    va_end(ap);\
    reset_log_name();\
    return puts(_log_buf);

class CTrpcTimeLog * g_ptrErrCallLog = NULL;
class CTrpcTimeLog * g_ptrTransApiCallLog = NULL;
class CTrpcTimeLog * g_ptrTransReqLog = NULL;
   
CTrpcTimeLog::CTrpcTimeLog(const char *log_base_path,
    int max_log_size,
    int max_log_num /* = 10 */,
    int * log_level_ptr /* = NULL */)
    :
    CLog(log_base_path, max_log_size, max_log_num, log_level_ptr),
    __msg_no(""),
    __current_date(0),
    __log_base(log_base_path)
{
    reset_log_name();
    __pid = getpid();
}

CTrpcTimeLog::~CTrpcTimeLog()
{
    close();
}

int
CTrpcTimeLog::reset_log_name()
{
    time_t timer;
    struct tm tm;
    timer = time(NULL);
    localtime_r(&timer, &tm);

    int now_date = (tm.tm_year + 1900) * 10000 + (tm.tm_mon + 1) * 100 + tm.tm_mday;

    if (__current_date == now_date)
    {
        return 0;
    }

    snprintf(_log_base_path, sizeof(_log_base_path), "%s_%08d_app",
             __log_base.c_str(), now_date);

    snprintf(_log_file_name, sizeof(_log_file_name), "%s.log",
             _log_base_path);    

    close();

    return 0;
}

int 
CTrpcTimeLog::in_log(const char *fmt, ...)
{
    if (get_log_level() < ERROR_LOG) {
        // 不写错误log
        return 0;
    }

    CFT_LOG_STR("in");
}

int 
CTrpcTimeLog::err_call_log(const char *fmt, ...)
{
    if (get_log_level() < ERROR_LOG) {
        // 不写错误log
        return 0;
    }

    CFT_LOG_STR("udf|err_call");
}
int 
CTrpcTimeLog::trans_api_log(const char * fmt, ...)
{
    if (get_log_level() < NORMAL_LOG) {
        return 0;
    }

    CFT_LOG_STR("trans_api");
}

int
CTrpcTimeLog::out_log(const char *fmt, ...)
{
    if (get_log_level() < ERROR_LOG) {
        // 不写错误log
        return 0;
    }

    CFT_LOG_STR("out");
}

int 
CTrpcTimeLog::rpc_log(const char *fmt, ...)
{
    if (get_log_level() < ERROR_LOG) {
        // 不写错误log
        return 0;
    }

    CFT_LOG_STR("rpc");
}

int 
CTrpcTimeLog::sql_log(const char *fmt, ...)
{
    if (get_log_level() < ERROR_LOG) {
        // 不写错误log
        return 0;
    }

    CFT_LOG_STR("sql");
}


// 重写基类函数

// 错误log，前面带 ERROR:
int 
CTrpcTimeLog::error_log(const char *fmt, ...)
{
    if (get_log_level() < ERROR_LOG) {
        // 不写错误log
        return 0;
    }

    CFT_LOG_STR("udf|error");
}

// 写普通log
int 
CTrpcTimeLog::write_log(const char *fmt, ...)
{
    if (get_log_level() < NORMAL_LOG) {
        // 不写错误log
        return 0;
    }

    CFT_LOG_STR("udf|normal");
}

// 调试log，前面带DEBUG:
int 
CTrpcTimeLog::debug_log(const char *fmt, ...)
{
    if (get_log_level() < DEBUG_LOG) {
        // 不写错误log
        return 0;
    }

    CFT_LOG_STR("udf|debug");
}

// 慢日志
int 
CTrpcTimeLog::slow_log(const char *fmt, ...)
{
    va_list ap;
    FILE *fp;
    char sLogFile[256];

    time_t timer;
    struct tm tm;
    timer = time(NULL);
    localtime_r(&timer, &tm);

    snprintf(sLogFile, sizeof(sLogFile), "%s_%04d%02d%02d_slow.log",
             __log_base.c_str(), tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

	// 判断文件是否超过2G, added by Red, 2004/2/14
	struct stat stStat;
	if (stat(sLogFile, &stStat) < 0) {
		if (errno != ENOENT) {
        	write_log("stat %s: %s\n", sLogFile, strerror(errno));
			return -1;
		}
	} else {
		if (stStat.st_size > (1024*1024*1800)) {
			error_log("Day log size too large: %lu\n", stStat.st_size);
			return -1;
		}
	}
	
    if ((fp = fopen(sLogFile, "a+")) == NULL) {
        write_log("fopen %s: %s\n", sLogFile, strerror(errno));
        return -1;
    }
    
    int ret = fprintf(fp, "%04d-%02d-%02d %02d:%02d:%02d|%s|%s|%d|",
                tm.tm_year + 1900, tm.tm_mon + 1,
                tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
                "slow", __msg_no.c_str(), __pid);
    
    if (ret < 0) {
        fclose(fp);
        return ret;
    }

    va_start(ap, fmt);
    ret = vfprintf(fp, fmt, ap);
    va_end(ap);

    if (ret < 0) {
        fclose(fp);
        return ret;
    }

    fclose(fp);

    return 0;
}

