#ifndef     __EVENT_THREAD_H__
#define     __EVENT_THREAD_H__

#include    <unistd.h>
#include    <pthread.h>

#include    "thread_mutex.h"
#include    "event_monitor.h"

class CEventThread
{
public:
    CEventThread();
    virtual ~CEventThread();

    virtual void init();
    virtual void fini();
    virtual void run();

    void    stop();
    int     heart_beat();
    void    set_heart_beat_handler(HeartBeatNotify * handler, void * param);

    void    add_event(int fd, int event, EventHandleFunc * event_handler, void * param,
                            int time_out);
    
    void    update_event(CEventBase * event_base, int event, EventHandleFunc * event_handler, 
                            void * param);
    
    void    del_event(CEventBase * & event_base);

    void    process_heart_beat();

    pthread_t   thread_id()
    {
        return _id;
    }

    bool    is_created() const
    {
        return _created;
    }

protected:
    static void * thread_run(void * arg);
    static void   heart_beat_notify(void * param);

protected:
    pthread_t       _id;

    CEventMonitor   _event_monitor;

    bool            _created;
    bool            _init;
    CWaitMutex      _init_mutex;

    int             _heart_beat;

    // 线程的处理函数
    HeartBeatNotify * _heart_beat_handler;
    void *          _heart_beat_param;
};

#endif

