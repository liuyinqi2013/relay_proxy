#ifndef __C_LOG_MUTEX_H__
#define __C_LOG_MUTEX_H__

#include "log.h"
#include "thread_mutex.h"

// �Ȳ��滻�����,���ּ���,�����ٸ�
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

