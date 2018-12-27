#include    "event_thread.h"
#include    <unistd.h>
#include    <stdlib.h>

CEventThread::CEventThread()
    :
    _created(false),
    _init(false),
    _heart_beat(0),
    _heart_beat_handler(NULL),
    _heart_beat_param(NULL)
{
}

CEventThread::~CEventThread()
{
}

// 执行线程特殊的初始化
void CEventThread::init()
{
    _event_monitor.init();
    
    _event_monitor.set_heart_beat_handler(CEventThread::heart_beat_notify, (void *)this);
    
    _init = true;
   
    _init_mutex.signal();
}

void CEventThread::fini()
{
    _init = false;

    _event_monitor.fini();

    // 线程结束
    _event_monitor.stop();
}

void CEventThread::run()
{
    // 启动线程
    if(pthread_create(&_id, NULL, CEventThread::thread_run, (void *)this) != 0) 
    {
        THROW(CThreadException, ERR_SYS_UNKNOWN, "pthread_create error: %s", strerror(errno));
    }
    
    _created = true ;

    // 等线程初始化完毕
    {
        CAutoLock lock(_init_mutex);

        while(!_init)
        {
            _init_mutex.wait();
        }
    }
}

void CEventThread::add_event(int fd, int event, EventHandleFunc * event_handler, void * param,
                                    int time_out)
{
    if (_event_monitor.add(fd, event, event_handler, param, time_out) != 0)
    {
        THROW(CTrpcException, ERR_SYS_UNKNOWN, "event add error: %s", _event_monitor.error_info());
    }
}

void CEventThread::update_event(CEventBase * event_base, int event, EventHandleFunc * event_handler, 
                        void * param)
{
    if (_event_monitor.update(event_base, event, event_handler, param) != 0)
    {
        THROW(CTrpcException, ERR_SYS_UNKNOWN, "event update error: %s", _event_monitor.error_info());
    }
}

void CEventThread::del_event(CEventBase * & event_base)
{
    DEBUG_TRACE();
    
    _event_monitor.del(event_base);
}

// 事件循环停止
void CEventThread::stop()
{
    _event_monitor.stop();
}

int CEventThread::heart_beat()
{
    return _heart_beat;
}

void CEventThread::set_heart_beat_handler(HeartBeatNotify * handler, void * param)
{
    _heart_beat_handler = handler;
    _heart_beat_param = param;
}

void CEventThread::process_heart_beat()
{
    _heart_beat = time(NULL);

    // 调用外部的处理函数
    if (_heart_beat_handler != NULL)
    {
        _heart_beat_handler(_heart_beat_param);
    }
}

void CEventThread::heart_beat_notify(void * param)
{
    CEventThread * me = (CEventThread *) param;

    me->process_heart_beat();  
}

void * CEventThread::thread_run(void * arg)
{    
    CEventThread * me = (CEventThread *) arg;
    
    try
    {
        // 初始化和清理外围控制
        // 初始化
        // me->init();   
        
        // 进入事件循环
        me->_event_monitor.monitor();

        // 清理
        // me->fini();

    } 
    catch (CTrpcException & ce)
    {
        /* do something */
        trpc_error_log("thread_run error %d, %s\n",
                            ce.error(), ce.error_info());
    } 
    catch (...)
    {
        /* do nothing */
    }

    // 线程出问题，直接exit
    exit(0);
    
    return ((void *)0);
}


