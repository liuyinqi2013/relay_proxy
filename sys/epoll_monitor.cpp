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
        // 出错
        // 设置错误信息
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
        // 修改event的信息，fd不做修改
        event_base->update(event, event_handler, param);

        // 更新epoll集合中的监听信息
        update_fd(event_base);
    }
    catch(CTrpcException& ce)
    {
        // 出错
        // 设置错误信息
        snprintf(_error_text, sizeof(_error_text), ce.error_info());
        
        return -1;
    }

    return 0;
}

void CEpollMonitor::add(CEventBase * event)
{
    // 增加到队列中
    _event_queue.add(event);

    // 增加到epoll集合中
    // 检查文件描述符是否正确
    struct stat statbuf;
    
    if (fstat(event->fd(), &statbuf) < 0)
    {
        THROW(CIpcException, ERR_SYS_UNKNOWN, "fd: %d, fstat error: %s", 
                    event->fd(), strerror(errno));
    }

    // 增加到epoll集合中
    add_fd(event);
}

void CEpollMonitor::del(CEventBase * & event)
{
    DEBUG_TRACE();

    CAutoLock lock(_mutex);

    // 如果已经不在队列中，为重复删除，直接返回
    if (!event->is_in_queue())
    {
        return;
    }
    
    // 检查obj类型,如果是永久存储,被删除应该属于代码意外,报错
    if (event->check_event(EV_PERSIST))
    {
        trpc_error_log("fd: %d, is persist, check again!!!", event->fd());

        return;
    }
    
    //CAutoLock lock(_mutex);

    trpc_debug_log("_event_queue.del fd: %d, event: %d", event->fd(), event->event());
    _event_queue.del(event);

    // 从epoll集合中删掉fd
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
    // 只会修改为stop，不会有其他变更，不加锁
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

    // 保持最大地址
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

// 重置时间类型
void CEpollMonitor::reset_event(CEventBase * event, int what)
{
    // 清空旧的事件类型
    event->clean_active();

    bool is_active = false;

    // 读
    if (what & EPOLLIN)
    {
        event->set_active(EV_READ);

        is_active = true;
    }

    // 写
    if (what & EPOLLOUT)
    {
        event->set_active(EV_WRITE);

        is_active = true;
    }

    // 超时
    // 获取当前时间
    struct  timeval now;
    
    memset(&now, 0x00, sizeof(now));
    gettimeofday(&now, NULL);

    // 出发超时
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

    // 调用处理函数
    event->handler_run();
}

void CEpollMonitor::add_fd(CEventBase * event)
{
    // 在epoll集合中增加fd
    struct epoll_event ev;  

    memset(&ev, 0x00, sizeof(struct epoll_event));

    // 根据是出或者入,设置event
    if (event->check_event(EV_READ))
    {
        ev.events |= EPOLLIN;
    }

    if (event->check_event(EV_WRITE))
    {
        ev.events |= EPOLLOUT;
    }

    // 设置fd
    ev.data.fd = event->fd();

    // 设置回调指针
    ev.data.ptr = event;

    trpc_debug_log("add fd: %d, event: %d, epoll_fd: %d", event->fd(), event->event(), _epoll_fd);

    // 加入epoll集合
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, event->fd(), &ev) < 0)
    {
        // 加入出错的话,关闭fd的事情交给外面来做
        THROW(CIpcException, ERR_SYS_UNKNOWN, "epoll_ctl add error: %s", strerror(errno));
    }
}

void CEpollMonitor::update_fd(CEventBase * event)
{
    // 在epoll集合中增加fd
    struct epoll_event ev;  

    memset(&ev, 0x00, sizeof(struct epoll_event));

    // 根据是出或者入,设置event
    if (event->check_event(EV_READ))
    {
        ev.events |= EPOLLIN;
    }

    if (event->check_event(EV_WRITE))
    {
        ev.events |= EPOLLOUT;
    }

    // 设置fd
    ev.data.fd = event->fd();

    // 设置回调指针
    ev.data.ptr = event;

    // 修改epoll集合中的监听事件类型
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, event->fd(), &ev) < 0)
    {
        // 加入出错的话,关闭fd的事情交给外面来做
        THROW(CIpcException, ERR_SYS_UNKNOWN, "epoll_ctl error: %s", strerror(errno));
    }
}

void CEpollMonitor::del_fd(CEventBase * event)
{
    // 在epoll集合删除
    struct epoll_event ev;  

    memset(&ev, 0x00, sizeof(struct epoll_event));

    // 根据是出或者入,设置event
    if (event->check_event(EV_READ))
    {
        ev.events |= EPOLLIN;
    }

    if (event->check_event(EV_WRITE))
    {
        ev.events |= EPOLLOUT;
    }

    // 设置fd
    ev.data.fd = event->fd();

    // 设置回调指针
    ev.data.ptr = event;

    // 加入epoll集合
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, event->fd(), &ev) < 0)
    {
        // epoll 删除出错一般都是外面socket被关闭,可以直接跳过,不影响后续处理   
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
    // event为空的话直接退出,不应该出现这种情况
    if (_event_queue.size() == 0)
    {
        trpc_error_log("event_queue is empty, wait queue: %d", _event_queue.size());

        exit(-1);
    }

    // 清空event
    memset(_events, 0x00, sizeof(struct epoll_event) * _epoll_size);
    
    // 返回的数量
    int nfds = epoll_wait(_epoll_fd, _events, _epoll_size, _epoll_time_out);
    
    if (nfds >= 0)
    {    
        for (int i = 0; i < nfds; ++i)
        {
            // 检查是读或者写触发
            CEventBase * p = (CEventBase *)_events[i].data.ptr;
            int what = _events[i].events;
            
            // 处理事件请求
            process(p, what);
        }
    }
    else
    {
        // 小于0，先打印错误日志看看
        trpc_error_log("epoll error: %s", strerror(errno));
    }

    ++_heart_beat_count;

    // 超时也不一定处理，处理一定请求检查一次 一般为32 * 20 ms = 0.65s左右
    // if (ret == 0 || _heart_beat_count > DEFAULT_BEAT_COUNT)
    if (_heart_beat_count > DEFAULT_BEAT_COUNT)
    {
        // 重置
        _heart_beat_count = 0;
        
        if (_heart_beat_handler != NULL)
        {
            // 调用心跳函数 -- 定期上报一些数据等处理
            _heart_beat_handler(_heart_beat_param);
        }
    }
}


