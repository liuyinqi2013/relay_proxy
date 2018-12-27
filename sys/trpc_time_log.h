#ifndef     __TRPC_TIME_LOG_H__
#define     __TRPC_TIME_LOG_H__

#include <stdio.h>
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string>

// 采用middle的日志来处理
#include "log.h"

/**
 * 日志类
 * 从middle的日志库派生来
 */
class CTrpcTimeLog : public CLog
{
public:
    CTrpcTimeLog(const char *log_base_path,
        int max_log_size,
        int max_log_num = 10,
        int * log_level_ptr = NULL);

    virtual ~CTrpcTimeLog();

public:
    /**
     * 设置日志级别 -- 用g_pLog的日志级别指针
     */
    inline void setMsgNo(const std::string & msg_no)
    { __msg_no = msg_no;}

protected:
    int reset_log_name();
    
public:
    int in_log(const char *fmt, ...);
    int out_log(const char *fmt, ...);
    int rpc_log(const char *fmt, ...);
    int sql_log(const char *fmt, ...);
    
    int err_call_log(const char *fmt, ...);

    int trans_api_log(const char * fmt, ...);
    
public:
    // 重写基类函数
    
    // 错误log，前面带 ERROR:
    int error_log(const char *fmt, ...);

    // 写普通log
    int write_log(const char *fmt, ...);

    // 调试log，前面带DEBUG:
    int debug_log(const char *fmt, ...);
    
public:
    int slow_log(const char *fmt, ...);

protected:
    std::string     __msg_no;
    
    pid_t           __pid;

    //  当前的天
    int             __current_date;

    std::string     __log_base;
};

/**
 * 日志操作宏(必须外部初始化g_ptrErrCallLog变量)
 */
extern CTrpcTimeLog* g_ptrErrCallLog; // 应用日志句柄

extern CTrpcTimeLog* g_ptrTransApiCallLog; // trans_api调用结果日志 

extern CTrpcTimeLog* g_ptrTransReqLog;  // 事务调用请求日志

#define TRPC_ERR_CALL_LOG g_ptrErrCallLog->err_call_log
#define TRPC_TRANS_API_LOG g_ptrTransApiCallLog->trans_api_log
#define TRPC_TRANS_REQ_LOG g_ptrTransReqLog->trans_api_log

#endif

