/*
	Ïß³Ì»¥³â
*/

#include "thread_mutex.h"

/**************thread mutex***************/
CThreadMutex::CThreadMutex()
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	
	pthread_mutex_init(&__mutex, &attr);

    memset(__error_text, 0x00, sizeof(__error_text));
}

CThreadMutex::~CThreadMutex()
{
	pthread_mutex_destroy(&__mutex);
}

int CThreadMutex::lock() 
{
    if (pthread_mutex_lock(&__mutex) != 0) 
    {
        snprintf(__error_text, sizeof(__error_text), "pthread_mutex_lock: %s", strerror(errno));
        return -1;
    }
    
    return 0;
}

int CThreadMutex::unlock() 
{
    if (pthread_mutex_unlock(&__mutex) != 0) 
    {
        snprintf(__error_text, sizeof(__error_text), "pthread_mutex_lock: %s", strerror(errno));
        return -1;
    }
    
    return 0;
}

pthread_mutex_t* CThreadMutex::mutex()
{
	return &__mutex;
}

/**************thread wait ond***************/
CWaitMutex::CWaitMutex()
{
    pthread_cond_init(&__cond, NULL);

    memset(__error_text, 0x00, sizeof(__error_text));
}

CWaitMutex::~CWaitMutex()
{
	pthread_cond_destroy(&__cond);
}

void CWaitMutex::wait()
{
	pthread_cond_wait(&__cond, &__mutex);
}

// ³¬Ê±wait
int CWaitMutex::wait(int sec, int nsec /* = 0 */)
{
    int ret = 0;
    struct timespec dst_time;

    dst_time.tv_sec = time(NULL) + sec;
    dst_time.tv_nsec = nsec;

    ret = pthread_cond_timedwait(&__cond, &__mutex, &dst_time);
    if (ret == 0)
    {
        return 0;
    }
    else if (ret != ETIMEDOUT)
    {
        snprintf(__error_text, sizeof(__error_text), "pthread_cond_timedwait: %s", strerror(errno));

        return -1;
    }
    else if (ret == ETIMEDOUT)
    {
        return 1;
    }

    return 0;
}

void CWaitMutex::signal() 
{
	pthread_cond_signal(&__cond);
}


/**************auto lock***************/
CAutoLock::CAutoLock(CThreadMutex& obj)
{
	__ptr = &obj;
	__ptr->lock();
}

CAutoLock::~CAutoLock()
{
	__ptr->unlock();
}

/**************rw lock***************/
CRwLock::CRwLock()
{
    pthread_rwlock_init(&__lock, NULL);
}
    
CRwLock::~CRwLock()
{
    pthread_rwlock_destroy(&__lock);
}

void CRwLock::rlock()
{
    pthread_rwlock_rdlock(&__lock);
}

void CRwLock::wlock()
{
    pthread_rwlock_wrlock(&__lock);
}

void CRwLock::unlock()
{
    pthread_rwlock_unlock(&__lock);
}

/**************auto rlock***************/
CAutoRLock::CAutoRLock(CRwLock& obj)
{
	__ptr = &obj;
	__ptr->rlock();
}

CAutoRLock::~CAutoRLock()
{
	__ptr->unlock();
}


/**************auto wlock***************/
CAutoWLock::CAutoWLock(CRwLock& obj)
{
	__ptr = &obj;
	__ptr->wlock();
}

CAutoWLock::~CAutoWLock()
{
	__ptr->unlock();
}


