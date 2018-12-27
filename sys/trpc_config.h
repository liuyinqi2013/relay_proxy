#ifndef     __TRPC_CONFIG_H__
#define     __TRPC_CONFIG_H__

#include    <stdio.h>

#include    "trpc_comm_func.h"
#include    "thread_mutex.h"

// 最大单行配置
static const int MAX_CONF_LENGTH_SIZE = 4196;

class CConfig
{
public:
    CConfig();
    virtual ~CConfig();

    virtual void read_conf(const char * conf_file,
            int (*fnGetRecord)(const char * str, void * arg) = CConfig::get_line_entrance,
            void * arg = NULL);

    // 清理
    void clear();

protected:
    // 子类实现 -- 这里留个定义，避免生成报错
    virtual int get_line(const char * str){ return -1; };
    virtual void _clear();

public:
    static int get_line_entrance(const char * str, void * arg);

protected:
    static const char * del_str;

protected:
    CRwLock _mutex;
    int     _rows;
};

#endif

