#ifndef     __EPOLL_MONITOR_H__
#define     __EPOLL_MONITOR_H__

#include 	<sys/epoll.h>

#include    <sys/time.h>
#include    <sys/types.h>
#include    <unistd.h>
#include    <vector>

#include    "event_queue.h"
#include    "thread_mutex.h"

/**
    处理事件循环,等待有响应触发相关处理函数
        1.  当新加入一个事件,先加入到待处理队列, 等待处理队列处理完毕, 
            再全部加入到处理队列中进行处理, 避免正在处理的事件,被重复加入,造成递归
        2.  事件处理函数调用完毕, 自动删除该事件, 如果需要再次处理, 需被重复加入
**/


class CEpollMonitor
{
public:
    CEpollMonitor();
    ~CEpollMonitor();

    int     add(int fd, int event, EventHandleFunc * event_handler, void * param = NULL, 
                        int time_out = CEventBase::DEFAULT_TIME_OUT);

    int     update(CEventBase * event_base, int event, EventHandleFunc * event_handler, void * param = NULL);

    void    del(CEventBase * & event);

    void    set_heart_beat_handler(HeartBeatNotify * handler, void * param);

    void    clean();

    void    stop();
    
    void    init();
    void    fini();

    void    reset_event(CEventBase * event, int what);

    // 循环
    void    monitor();
    
    void    monitor_once();

    void    process(CEventBase * event, int what);

    const char * error_info()
    {
        return _error_text;
    }

protected:
    void    add(CEventBase * event);

    // 在epoll集合中删除和增加fd
    void    add_fd(CEventBase * event);
    void    del_fd(CEventBase * event);
    void    update_fd(CEventBase * event);

protected:
    // 默认的epoll最长时间单位 ms
    static const int DEFAULT_WAIT_TIME = 2000;

    // 最长情况下，0.5秒超市一次
    static const int DEFAULT_BEAT_COUNT = 32;

    // 最大的epoll size 6w足够fd最大也差不多
    static const int DEFAULT_MAX_EPOLL_SIZE = 60000;

    static const int MAX_ERRMSG_LEN = 256;

protected:
    // 正在事件处理队列
    CEventDecQueue  _event_queue;
    
    CThreadMutex    _mutex;

    bool            _stop;
	
	int 			_epoll_fd;
    int             _epoll_time_out;
    int             _epoll_size;
    struct epoll_event * _events; 
    
    int             _heart_beat_count;
    HeartBeatNotify *   _heart_beat_handler;
    void *          _heart_beat_param;

    // 出错信息
    char            _error_text[MAX_ERRMSG_LEN];
};


#endif

