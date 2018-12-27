/*
   �̹߳������ݻ���
*/

#ifndef __THREAD_MUTEX_H__
#define __THREAD_MUTEX_H__

#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

class CThreadMutex
{
public:
    CThreadMutex();
    virtual ~CThreadMutex();

public:
    const char * get_error_text() const {return __error_text;}
    
    int lock();
    int unlock();
    
    pthread_mutex_t* mutex();
    
protected:
    pthread_mutex_t __mutex;
    
    char __error_text[256];
};

class CWaitMutex : public CThreadMutex
{
public:
    CWaitMutex();
    virtual ~CWaitMutex();
    
    void wait();
    int wait(int sec, int nsec = 0);
    
    void signal();
    
protected:
    pthread_cond_t __cond;
};

class CAutoLock
{
public:
    CAutoLock(CThreadMutex& ptr);
    ~CAutoLock();
    
protected:
    CThreadMutex* __ptr;
};

// ��д��
class CRwLock
{
public:
    CRwLock();
    ~CRwLock();

    void rlock();
    void wlock();
    void unlock();

private:
    pthread_rwlock_t __lock;
};

// �Խ����
class CAutoRLock
{
public:
    CAutoRLock(CRwLock& ptr);
    ~CAutoRLock();
    
protected:
    CRwLock* __ptr;    
};

// �Խ�д��
class CAutoWLock
{
public:
    CAutoWLock(CRwLock& ptr);
    ~CAutoWLock();
    
protected:
    CRwLock* __ptr;   
};

#endif

