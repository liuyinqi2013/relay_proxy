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
    �����¼�ѭ��,�ȴ�����Ӧ������ش�����
        1.  ���¼���һ���¼�,�ȼ��뵽���������, �ȴ�������д������, 
            ��ȫ�����뵽��������н��д���, �������ڴ�����¼�,���ظ�����,��ɵݹ�
        2.  �¼��������������, �Զ�ɾ�����¼�, �����Ҫ�ٴδ���, �豻�ظ�����
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

    // ѭ��
    void    monitor();
    
    void    monitor_once();

    void    process(CEventBase * event, int what);

    const char * error_info()
    {
        return _error_text;
    }

protected:
    void    add(CEventBase * event);

    // ��epoll������ɾ��������fd
    void    add_fd(CEventBase * event);
    void    del_fd(CEventBase * event);
    void    update_fd(CEventBase * event);

protected:
    // Ĭ�ϵ�epoll�ʱ�䵥λ ms
    static const int DEFAULT_WAIT_TIME = 2000;

    // �����£�0.5�볬��һ��
    static const int DEFAULT_BEAT_COUNT = 32;

    // ����epoll size 6w�㹻fd���Ҳ���
    static const int DEFAULT_MAX_EPOLL_SIZE = 60000;

    static const int MAX_ERRMSG_LEN = 256;

protected:
    // �����¼��������
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

    // ������Ϣ
    char            _error_text[MAX_ERRMSG_LEN];
};


#endif

