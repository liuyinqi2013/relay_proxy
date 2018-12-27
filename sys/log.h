#ifndef __C_LOG_H
#define __C_LOG_H

/*
功能: 
       1.  Log类

Created by Song, 2003-01
Change list:

*/


#include    <stdio.h>
#include    <unistd.h> 
#include    <sys/syscall.h> 
#include    <sys/types.h> 

// tlinux 总找不到头文件，单独搞一个
inline pid_t gettid() 
{ 
    return syscall(SYS_gettid); 
} 

class CBaseLogMutex
{
public:
    virtual int Lock() = 0;
    virtual int UnLock() = 0;
public:
    virtual ~CBaseLogMutex(){};
};

class CLog
{
public:
    CLog(const char *log_base_path,
        int max_log_size,
        int max_log_num = 10,
        int * log_level_ptr = NULL);
    
    virtual ~CLog();

public:
    int open();
    int close();
    const char * get_error() const { return _error_text; }

public:
    enum CLogLevel
    {
        NO_LOG = 1,  // 所有的log都不写
        ERROR_LOG = 2,  // 只写错误log
        NORMAL_LOG = 3,  // 写普通log和错误log
        DEBUG_LOG = 4  // 写普通log、错误log和调试log
    };

    // 取得log的级别
    inline int get_log_level() const 
    {return _log_level_ptr != NULL ? (*_log_level_ptr):DEBUG_LOG;}

    inline int * get_log_level_ptr()
    {return (int *)_log_level_ptr;}

    inline void set_log_level_ptr(int * log_level_ptr)
    {_log_level_ptr = log_level_ptr;}

    static char * get_log_level_str(int log_level, char * str);
    static bool log_level_is_valid(int log_level);

    // 设置互斥锁
    void set_log_mutex(CBaseLogMutex * log_mutex){_log_mutex = log_mutex;}
public:
    // 多进程时显示进程编号
    void set_process_no(int proc_no) { _proc_no = proc_no; }
    // 多进程时显示进程编号
    int get_process_no() { return _proc_no; }

    // 错误log，前面带 ERROR:
    int error_log(const char *fmt, ...);

    // 写普通log
    int write_log(const char *fmt, ...);

    // 调试log，前面带DEBUG:
    int debug_log(const char *fmt, ...);

    // 以16进制形式打印buf
    int print_hex(const void *buf, size_t len);

    // 不建议使用
    int puts(const char * str);

    // 每天一个的log
    int day_log(const char * day_log_path, const char *fmt, ...);

    // 和day_log类似，但前面不写时间
    int day_log_raw(const char * day_log_path, const char *fmt, ...);

private:
    int _puts(const char *str);
    int shilft_files();
    inline void Lock()
    {
        if (_log_mutex != NULL){
            _log_mutex->Lock();
        }
    }
    
    inline void UnLock()
    {
        if (_log_mutex != NULL){
            _log_mutex->UnLock();
        }
    }
    
protected:
    static const int LOG_SHIFT_COUNT  = 32;

    FILE * _fp;

    char _log_base_path[256];
    CBaseLogMutex * _log_mutex;
    int _max_log_size;
    int _max_log_num;

    // log的级别指针，通常指向共享内存
    // 当这个指针为NULL时，取默认NORMAL_LOG
    volatile int * _log_level_ptr;

    char _log_file_name[256];
    
    int _write_count;
    int _proc_no;
    
    char _log_buf[16384];
    char _error_text[256];
};

extern class CLog * g_pLog;
extern class CLog * g_pAccLog;
extern class CLog * g_pStatsLog;


// changed by verminniu 2008-03-25
// 增加acclog
#ifdef DEBUG_LOG_ARGS
    #define trpc_write_log printf
    #define trpc_error_log printf
    #define trpc_debug_log printf

    #define trpc_write_acclog printf
    #define trpc_error_acclog printf
    #define trpc_debug_acclog printf

    #define trpc_write_statslog printf
    #define trpc_error_statsllog printf
    #define trpc_debug_statsllog printf
#else
    
    // #define trpc_write_log g_pLog->write_log
    // #define trpc_error_log g_pLog->error_log
    // #define trpc_debug_log g_pLog->debug_log
    
    //////////以下测试用的日志
    #define     __TRPC_MIDDLE_SERVER_MAX_LOG_PER_ROW__ 4096
    
    #define     trpc_debug_log(...) {\
                char __MIDDLE_LOG_BUF__[__TRPC_MIDDLE_SERVER_MAX_LOG_PER_ROW__] = {0}; \
                snprintf(__MIDDLE_LOG_BUF__, sizeof(__MIDDLE_LOG_BUF__) - 1, __VA_ARGS__ );\
                g_pLog->debug_log("[%s][%d]:%s\n", __FILE__, __LINE__, __MIDDLE_LOG_BUF__);}
    
    #define     trpc_error_log(...)  {\
                char __MIDDLE_LOG_BUF__[__TRPC_MIDDLE_SERVER_MAX_LOG_PER_ROW__] = {0}; \
                snprintf(__MIDDLE_LOG_BUF__, sizeof(__MIDDLE_LOG_BUF__) - 1, __VA_ARGS__ );\
                g_pLog->error_log("[%s][%d]:%s\n", __FILE__, __LINE__, __MIDDLE_LOG_BUF__);}
    
    #define     trpc_write_log(...)  {\
                char __MIDDLE_LOG_BUF__[__TRPC_MIDDLE_SERVER_MAX_LOG_PER_ROW__] = {0}; \
                snprintf(__MIDDLE_LOG_BUF__, sizeof(__MIDDLE_LOG_BUF__) - 1, __VA_ARGS__ );\
                g_pLog->write_log("[%s][%d]:%s\n", __FILE__, __LINE__, __MIDDLE_LOG_BUF__);}
    ////////////

    #define trpc_write_acclog g_pAccLog->write_log
    #define trpc_error_acclog g_pAccLog->error_log
    #define trpc_debug_acclog g_pAccLog->debug_log

    #define trpc_write_statslog g_pStatsLog->write_log
    #define trpc_error_statslog g_pStatsLog->error_log
    #define trpc_debug_statslog g_pStatsLog->debug_log
#endif


#define trpc_set_log_mutex g_pLog->set_log_mutex

#define trpc_set_acclog_mutex g_pAccLog->set_log_mutex

#define trpc_set_statslog_mutex g_pStatsLog->set_log_mutex

#ifdef DEBUG
    // 测试调试信息
    #define DEBUG_TEST(...) fprintf(stderr, __VA_ARGS__)

    //#define DEBUG_TRACE() trpc_debug_log("file: %s, line: %d, func: %s", __FILE__, __LINE__, __func__)
    #define DEBUG_TRACE()
#else
    #define DEBUG_TEST(...)
    #define DEBUG_TRACE()
#endif

#endif
