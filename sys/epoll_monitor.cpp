#include    <sys/stat.h>
#include    <unistd.h>
#include    <stdlib.h>

#include    "epoll_monitor.h"
#include    "trpc_socket_opt.h"

CEpollMonitor::CEpollMonitor()
    :
    _stop(false),
    _epoll_fd(INVALID_SOCKET),
    _epoll_time_out(DEFAULT_WAIT_TIME),
    _epoll_size(DEFAULT_MAX_EPOLL_SIZE),
    _events(NULL),
    _heart_beat_count(0),
    _heart_beat_handler(NULL),
    _heart_beat_param(NULL)
{
}

CEpollMonitor::~CEpollMonitor()
{
    clean();
}

void CEpollMonitor::set_heart_beat_handler(HeartBeatNotify * handler, void * param)
{
    _heart_beat_handler = handler;
    _heart_beat_param = param;
}

int CEpollMonitor::add(int fd, int event, EventHandleFunc * event_handler, void * param /* = NULL */, 
                                int time_out /* = CEventBase::DEFAULT_TIME_OUT */)
{
    CEventBase * obj = new CEventBase(fd, event, event_handler, param, time_out);

    assert(obj != NULL);

    CAutoLock lock(_mutex);

    try
    {
        add(obj);
    }
    catch(CTrpcException& ce)
    {
        // ����
        // ���ô�����Ϣ
        snprintf(_error_text, sizeof(_error_text), ce.error_info());
        
        return -1;
    }

    return 0;
}

int CEpollMonitor::update(CEventBase * event_base, int event, EventHandleFunc * event_handler, 
                        void * param)
{
    CAutoLock lock(_mutex);

    try
    {
        // �޸�event����Ϣ��fd�����޸�
        event_base->update(event, event_handler, param);

        // ����epoll�����еļ�����Ϣ
        update_fd(event_base);
    }
    catch(CTrpcException& ce)
    {
        // ����
        // ���ô�����Ϣ
        snprintf(_error_text, sizeof(_error_text), ce.error_info());
        
        return -1;
    }

    return 0;
}

void CEpollMonitor::add(CEventBase * event)
{
    // ���ӵ�������
    _event_queue.add(event);

    // ���ӵ�epoll������
    // ����ļ��������Ƿ���ȷ
    struct stat statbuf;
    
    if (fstat(event->fd(), &statbuf) < 0)
    {
        THROW(CIpcException, ERR_SYS_UNKNOWN, "fd: %d, fstat error: %s", 
                    event->fd(), strerror(errno));
    }

    // ���ӵ�epoll������
    add_fd(event);
}

void CEpollMonitor::del(CEventBase * & event)
{
    DEBUG_TRACE();

    CAutoLock lock(_mutex);

    // ����Ѿ����ڶ����У�Ϊ�ظ�ɾ����ֱ�ӷ���
    if (!event->is_in_queue())
    {
        return;
    }
    
    // ���obj����,��������ô洢,��ɾ��Ӧ�����ڴ�������,����
    if (event->check_event(EV_PERSIST))
    {
        trpc_error_log("fd: %d, is persist, check again!!!", event->fd());

        return;
    }
    
    //CAutoLock lock(_mutex);

    trpc_debug_log("_event_queue.del fd: %d, event: %d", event->fd(), event->event());
    _event_queue.del(event);

    // ��epoll������ɾ��fd
    del_fd(event);

    delete event;

    event = NULL;
}

void CEpollMonitor::clean()
{
    _event_queue.clean();
}

void CEpollMonitor::stop()
{
    // ֻ���޸�Ϊstop�����������������������
    _stop = true;
}

void CEpollMonitor::init()
{
    assert(_epoll_fd == INVALID_SOCKET);
    
    _epoll_fd = epoll_create(_epoll_size);
    
    if (_epoll_fd < 0)
    {
        THROW(CIpcException, ERR_SYS_UNKNOWN, "select monitor INVALID_SOCKET");
    }

    // ��������ַ
    _events = new struct epoll_event[_epoll_size];
}

void CEpollMonitor::fini()
{
    // delete event bufs
    delete []_events;
    _events = NULL;

    // close _epoll_fd
    close(_epoll_fd);
    _epoll_fd = INVALID_SOCKET;

    clean();
}

// ����ʱ������
void CEpollMonitor::reset_event(CEventBase * event, int what)
{
    // ��վɵ��¼�����
    event->clean_active();

    bool is_active = false;

    // ��
    if (what & EPOLLIN)
    {
        event->set_active(EV_READ);

        is_active = true;
    }

    // д
    if (what & EPOLLOUT)
    {
        event->set_active(EV_WRITE);

        is_active = true;
    }

    // ��ʱ
    // ��ȡ��ǰʱ��
    struct  timeval now;
    
    memset(&now, 0x00, sizeof(now));
    gettimeofday(&now, NULL);

    // ������ʱ
    if (!is_active)
    {
        if (event->is_time_out(&now))
        {
            event->set_active(EV_TIMEOUT);
        }     
    }
}

void CEpollMonitor::process(CEventBase * event, int what)
{
    reset_event(event, what);

    // ���ô�����
    event->handler_run();
}

void CEpollMonitor::add_fd(CEventBase * event)
{
    // ��epoll����������fd
    struct epoll_event ev;  

    memset(&ev, 0x00, sizeof(struct epoll_event));

    // �����ǳ�������,����event
    if (event->check_event(EV_READ))
    {
        ev.events |= EPOLLIN;
    }

    if (event->check_event(EV_WRITE))
    {
        ev.events |= EPOLLOUT;
    }

    // ����fd
    ev.data.fd = event->fd();

    // ���ûص�ָ��
    ev.data.ptr = event;

    trpc_debug_log("add fd: %d, event: %d, epoll_fd: %d", event->fd(), event->event(), _epoll_fd);

    // ����epoll����
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, event->fd(), &ev) < 0)
    {
        // �������Ļ�,�ر�fd�����齻����������
        THROW(CIpcException, ERR_SYS_UNKNOWN, "epoll_ctl add error: %s", strerror(errno));
    }
}

void CEpollMonitor::update_fd(CEventBase * event)
{
    // ��epoll����������fd
    struct epoll_event ev;  

    memset(&ev, 0x00, sizeof(struct epoll_event));

    // �����ǳ�������,����event
    if (event->check_event(EV_READ))
    {
        ev.events |= EPOLLIN;
    }

    if (event->check_event(EV_WRITE))
    {
        ev.events |= EPOLLOUT;
    }

    // ����fd
    ev.data.fd = event->fd();

    // ���ûص�ָ��
    ev.data.ptr = event;

    // �޸�epoll�����еļ����¼�����
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, event->fd(), &ev) < 0)
    {
        // �������Ļ�,�ر�fd�����齻����������
        THROW(CIpcException, ERR_SYS_UNKNOWN, "epoll_ctl error: %s", strerror(errno));
    }
}

void CEpollMonitor::del_fd(CEventBase * event)
{
    // ��epoll����ɾ��
    struct epoll_event ev;  

    memset(&ev, 0x00, sizeof(struct epoll_event));

    // �����ǳ�������,����event
    if (event->check_event(EV_READ))
    {
        ev.events |= EPOLLIN;
    }

    if (event->check_event(EV_WRITE))
    {
        ev.events |= EPOLLOUT;
    }

    // ����fd
    ev.data.fd = event->fd();

    // ���ûص�ָ��
    ev.data.ptr = event;

    // ����epoll����
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, event->fd(), &ev) < 0)
    {
        // epoll ɾ������һ�㶼������socket���ر�,����ֱ������,��Ӱ���������   
        trpc_debug_log("epoll_ctl del fd: %d, error: %s", event->fd(), strerror(errno));
    }
}

void CEpollMonitor::monitor()
{
    while(!_stop)
    {
        try
        {
            monitor_once();
        }
        catch(CTrpcException& ce)
        {
            trpc_error_log("CTrpcException: %s", ce.error_info());
            
            throw;
        }
        catch(std::exception& ce)
        {
            trpc_error_log("std::exception: %s", ce.what());
            
            throw;
        }
        catch(...)
        {
            trpc_error_log("unknown exception");

            throw;
        }
    }
}

void CEpollMonitor::monitor_once()
{
    // eventΪ�յĻ�ֱ���˳�,��Ӧ�ó����������
    if (_event_queue.size() == 0)
    {
        trpc_error_log("event_queue is empty, wait queue: %d", _event_queue.size());

        exit(-1);
    }

    // ���event
    memset(_events, 0x00, sizeof(struct epoll_event) * _epoll_size);
    
    // ���ص�����
    int nfds = epoll_wait(_epoll_fd, _events, _epoll_size, _epoll_time_out);
    
    if (nfds >= 0)
    {    
        for (int i = 0; i < nfds; ++i)
        {
            // ����Ƕ�����д����
            CEventBase * p = (CEventBase *)_events[i].data.ptr;
            int what = _events[i].events;
            
            // �����¼�����
            process(p, what);
        }
    }
    else
    {
        // С��0���ȴ�ӡ������־����
        trpc_error_log("epoll error: %s", strerror(errno));
    }

    ++_heart_beat_count;

    // ��ʱҲ��һ����������һ��������һ�� һ��Ϊ32 * 20 ms = 0.65s����
    // if (ret == 0 || _heart_beat_count > DEFAULT_BEAT_COUNT)
    if (_heart_beat_count > DEFAULT_BEAT_COUNT)
    {
        // ����
        _heart_beat_count = 0;
        
        if (_heart_beat_handler != NULL)
        {
            // ������������ -- �����ϱ�һЩ���ݵȴ���
            _heart_beat_handler(_heart_beat_param);
        }
    }
}


