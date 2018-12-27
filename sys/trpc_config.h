#ifndef     __TRPC_CONFIG_H__
#define     __TRPC_CONFIG_H__

#include    <stdio.h>

#include    "trpc_comm_func.h"
#include    "thread_mutex.h"

// ���������
static const int MAX_CONF_LENGTH_SIZE = 4196;

class CConfig
{
public:
    CConfig();
    virtual ~CConfig();

    virtual void read_conf(const char * conf_file,
            int (*fnGetRecord)(const char * str, void * arg) = CConfig::get_line_entrance,
            void * arg = NULL);

    // ����
    void clear();

protected:
    // ����ʵ�� -- �����������壬�������ɱ���
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

