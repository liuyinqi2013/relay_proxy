#ifndef     __ACCESS_CONTROL_H__
#define     __ACCESS_CONTROL_H__

#include    <string>
#include    <vector>
#include    <set>
#include    <map>

#include    "thread_mutex.h"
#include    "trpc_config.h"

/*****
    ����ip���ƣ�������Դip�����ж�
        ��Ӧ��ǰ�����е�access_conf
****/

class CAccessControl: public CConfig
{
public:
    CAccessControl();
    virtual ~CAccessControl();

    void    check(const char * ip, const int request_type);
    void    check(const char * ip);
    
protected:
    virtual int get_line(const char * str);
    virtual void _clear();
    
    int     _get_line(const char * str);

    void    _check(const char * ip, const int request_type);
    void    _check(const char * ip);
    
public:
    static const int MAX_REQUEST_TYPE_PER_IP = 100;

protected:
    // ipΪkey��set Ϊrequest_type�б�
    std::map<std::string, std::set<int> >  _access_control;
};

#endif

