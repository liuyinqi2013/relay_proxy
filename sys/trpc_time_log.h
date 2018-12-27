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

// ����middle����־������
#include "log.h"

/**
 * ��־��
 * ��middle����־��������
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
     * ������־���� -- ��g_pLog����־����ָ��
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
    // ��д���ຯ��
    
    // ����log��ǰ��� ERROR:
    int error_log(const char *fmt, ...);

    // д��ͨlog
    int write_log(const char *fmt, ...);

    // ����log��ǰ���DEBUG:
    int debug_log(const char *fmt, ...);
    
public:
    int slow_log(const char *fmt, ...);

protected:
    std::string     __msg_no;
    
    pid_t           __pid;

    //  ��ǰ����
    int             __current_date;

    std::string     __log_base;
};

/**
 * ��־������(�����ⲿ��ʼ��g_ptrErrCallLog����)
 */
extern CTrpcTimeLog* g_ptrErrCallLog; // Ӧ����־���

extern CTrpcTimeLog* g_ptrTransApiCallLog; // trans_api���ý����־ 

extern CTrpcTimeLog* g_ptrTransReqLog;  // �������������־

#define TRPC_ERR_CALL_LOG g_ptrErrCallLog->err_call_log
#define TRPC_TRANS_API_LOG g_ptrTransApiCallLog->trans_api_log
#define TRPC_TRANS_REQ_LOG g_ptrTransReqLog->trans_api_log

#endif

