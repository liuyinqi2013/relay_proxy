#ifndef     __EVENT_QUEUE_H__
#define     __EVENT_QUEUE_H__

#include    "event_base.h"
#include    "trpc_sys_exception.h"

class CEventQueue
{
public:
    CEventQueue();
    ~CEventQueue();

    void add(CEventBase * value);
    void del(CEventBase * value);
    void insert(CEventBase * value, CEventBase * prev, CEventBase* next);

    void clean();

    CEventBase * begin();
    CEventBase * end();

    int size();
    
protected:
    CEventBase *    _head;
    CEventBase *    _tail;

    int             _size;
};

// 有序的事件队列
class CEventDecQueue: public CEventQueue
{
public:
    void add(CEventBase * value);
};

#endif

