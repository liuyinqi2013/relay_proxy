#ifndef __C_LOG_MUTEX_H__
#define __C_LOG_MUTEX_H__

#include "log.h"
#include "thread_mutex.h"

// 先不替换这个类,保持兼容,后面再改
class CLogMutex: public CBaseLogMutex
{
public:
    CLogMutex();
    virtual ~CLogMutex();

    const char * get_error_text() const {return _mutex.get_error_text();}

public:
    virtual int Lock();
    virtual int UnLock();

private:
    CThreadMutex _mutex;
};

#endif

