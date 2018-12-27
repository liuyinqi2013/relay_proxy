#include    "event_queue.h"

CEventQueue::CEventQueue()
    :
    _head(NULL),
    _tail(NULL),
    _size(0)
{
    // ����սڵ�, ����������β�ڵ�, ����ô���
    // tail ��fd����С��-1
    _tail = new CEventBase();

    _head = _tail;
}

CEventQueue::~CEventQueue()
{
    _head = NULL;
    if (_tail != NULL)
    {
        delete _tail;
        _tail = NULL;
    }

    _size = 0;
}

void CEventQueue::add(CEventBase * value)
{
    assert(value != NULL);
    assert( value->_in_queue == false);

    value->_next = _head;
    _head->_prev = value;
    _head = value;
    ++_size;

    value->_in_queue = true;
}

// ���뵽prev��next���м�
void CEventQueue::insert(CEventBase * value, CEventBase * prev, CEventBase* next)
{
    assert( value->_in_queue == false);
    assert(next != _head);
    assert(prev != _tail);
    
    value->_next = next;
    value->_prev = prev;

    prev->_next = value;
    next->_prev = value;

    value->_in_queue = true;

    ++_size;
}

void CEventQueue::del(CEventBase * value)
{
    assert(value != NULL);
    assert(value->_in_queue == true);

    value->_in_queue = false;

    assert(value != _tail);    // �������������,�ȱ���
    
    if (value == _head)
    {
        _head = value->_next;
        _head->_prev = NULL;
    }
    else
    {
        value->_next->_prev = value->_prev;
        value->_prev->_next = value->_next;
    }
    
    value->_next = NULL;
    value->_prev = NULL;
        
    --_size;
}

void CEventQueue::clean()
{
    while(size()!=0)
    {
        del(_head);
    }
}

CEventBase * CEventQueue::begin()
{
    return _head;
}

CEventBase * CEventQueue::end()
{
    return _tail;
}

int CEventQueue::size()
{
    return _size;
}

// ������¼����� -- ��max_fd����
void CEventDecQueue::add(CEventBase * value)
{
    CEventBase * p = begin();

    if (p == end())
    {
        // ֻ��һ����¼
        CEventQueue::add(value);
    }
    else if (p->fd() < value->fd())
    {
        // �ȴ�����Ҫ���뵽����ͷ�����
        CEventQueue::add(value);
    }
    else if (p->fd() == value->fd())
    {
        THROW(CIpcException, ERR_SYS_UNKNOWN, "equal fd: %d", value->fd());
    }
    else
    {
        while(p->next() != end() 
                    && p->next()->fd() > value->fd())
        {
            p = p->next();
        }

        // ����ǵ���,ֱ�ӱ�����,�϶������bug��,һ�������������������������ͬ��fd
        if (p->next()->fd() == value->fd())
        {
            THROW(CIpcException, ERR_SYS_UNKNOWN, "equal fd: %d", value->fd());
        }
        
        CEventQueue::insert(value, p, p->next());
    }
}
