#include    <sys/time.h>

#include    "trpc_socket_opt.h"
#include    "event_base.h"
#include    "log.h"

CEventBase::CEventBase()
    :
    _next(NULL),
    _prev(NULL),
    _fd(INVALID_SOCKET),
    _event(EV_NO_EVENT),
    _param(NULL),
    _handler(NULL)
{
}
    
CEventBase::CEventBase(int fd, int event, EventHandleFunc * handler, void * param, 
                            int time_out /* = DEFAULT_TIME_OUT */)
    :
    _next(NULL),
    _prev(NULL),
    _fd(fd),
    _event(event),
    _param(param),
    _handler(handler)
{
    init(fd, event, handler, param, time_out);
}

CEventBase::~CEventBase()
{
    _next = NULL;
    _prev = NULL;
    _fd = INVALID_SOCKET;
    _event = EV_NO_EVENT;
    _param = NULL;
    _handler = NULL;
}

int CEventBase::fd()
{
    return _fd;
}

int CEventBase::event()
{
    return _event;
}

int CEventBase::active_event()
{
    return _active_event;
}

void CEventBase::set_event(int event)
{
    set_mask(_event, event);
}

void CEventBase::unset_event(int event)
{
    unset_mask(_event, event);
}

void CEventBase::clean_event()
{
    clean_mask(_event);
}

bool CEventBase::check_event(int event)
{
    return check_mask(_event, event);
}

bool CEventBase::is_in_queue()
{
    return _in_queue;
}

void CEventBase::set_active(int event)
{
    set_mask(_active_event, event);
}

void CEventBase::unset_active(int event)
{
    unset_mask(_active_event, event);
}

void CEventBase::clean_active()
{
    clean_mask(_active_event);
}

bool CEventBase::check_active(int event)
{
    return check_mask(_active_event, event);
}

void CEventBase::init(int fd, int event, EventHandleFunc * handler, void * param,
                            int time_out /* = DEFAULT_TIME_OUT */)
{
    _fd     = fd;
    _event  = event;
    _handler = handler;
    _param  = param;

    gettimeofday(&_init_time, 0);
    
    // ����ĵ�λ��ms��������us, ������ĳ�ʱʱ����0������Ĭ��
    if (time_out <= 0)
    {
        _time_out = DEFAULT_TIME_OUT * 1000;
    }
    else
    {
        _time_out = time_out * 1000;
    }

    _in_queue = false;
}

void CEventBase::update(int event, EventHandleFunc * handler, void * param)
{
    _event = event;
    _handler = handler;
    _param = param;

    // ���ó�ʼ��ʱ�� -- ÿ�μӽ�����ʵ��һ����������ʱ�����ܵľͿ�����
}

void CEventBase::handler_run()
{
    // ֻ����Ҫ���òŽ��д���
    if (_active_event & _event)
    {
        _handler(_fd, _active_event, _param, this);
    }
}

CEventBase * CEventBase::next()
{
    return _next;
}

CEventBase * CEventBase::prev()
{
    return _prev;
}

bool CEventBase::is_time_out(const struct timeval * now)
{
    // �������Զ���ڵĻ������ǳ�ʱ
    if (check_event(EV_PERSIST))
    {
        return false;
    }
    
    // _time_out��λΪus
    int us = (now->tv_sec - _init_time.tv_sec) * 1000000 
                + now->tv_usec - _init_time.tv_usec;

    return us > _time_out;
}

