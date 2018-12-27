#ifndef     __EVENT_BASE_H__
#define     __EVENT_BASE_H__

#include    "trpc_comm_func.h"

/**
    event的基类,提供事件响应的接口,按链表方式组织
**/

// 定义先支持,但是现在暂时不支持全双工socket
#define EV_TIMEOUT	0x01
#define EV_READ		0x02 
#define EV_WRITE	0x04 
#define EV_SIGNAL	0x08 
#define EV_PERSIST	0x10	/* Persistant event */

#define EV_NO_EVENT 0x00      // 无需要响应的事件 -- 从队列中删除

class CEventQueue;
class CEventBase;

typedef int EventHandleFunc(int fd, int event, void * param, CEventBase * event_base);

// 用来通知monitor循环正在正常运行
typedef void HeartBeatNotify(void * param);

class CEventBase
{
public:
    CEventBase();
    CEventBase(int fd, int event, EventHandleFunc * handler, void * param, 
                            int time_out = DEFAULT_TIME_OUT);
    
    virtual ~CEventBase();

    int     fd();
    int     event();
    int     active_event();

    void    set_event(int event);
    void    unset_event(int event);   
    void    clean_event();
    bool    check_event(int event);

    void    set_active(int event);
    void    unset_active(int event);   
    void    clean_active();
    bool    check_active(int event);

    bool    is_in_queue();
    
    bool    is_time_out(const struct timeval * now);
    
    void    init(int fd, int event, EventHandleFunc * handler, void * param, 
                    int time_out = DEFAULT_TIME_OUT);

    void    update(int event, EventHandleFunc * handler, void * param);
    
    void    handler_run();

    CEventBase * next();
    CEventBase * prev();
    
public:
    // 声明为友元
    friend class CEventQueue;

public:
    // 默认超时时间2秒
    static const int DEFAULT_TIME_OUT = 2;

protected:
    CEventBase *    _next;
    CEventBase *    _prev;

    int             _fd;
    int             _event;     // 需要触发的event
    void *          _param;

    bool            _in_queue;   // 检查是否在事件队列中

    int             _active_event;  // 已经触发的事件类型

    int             _time_out;
    struct timeval  _init_time;

    EventHandleFunc *   _handler;
};

#endif

