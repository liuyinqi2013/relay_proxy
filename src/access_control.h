#ifndef     __ACCESS_CONTROL_H__
#define     __ACCESS_CONTROL_H__

#include    <string>
#include    <vector>
#include    <set>
#include    <map>

#include    "thread_mutex.h"
#include    "trpc_config.h"

/*****
    进行ip限制，根据来源ip进行判断
        对应以前配置中的access_conf
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
    // ip为key，set 为request_type列表
    std::map<std::string, std::set<int> >  _access_control;
};

#endif

