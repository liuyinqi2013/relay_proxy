#include    "access_control.h"
#include    "trpc_sys_exception.h"
#include <stdlib.h>

CAccessControl::CAccessControl()
{
}

CAccessControl::~CAccessControl()
{
}

void CAccessControl::_clear()
{
    _access_control.clear();
}


int CAccessControl::get_line(const char * str)
{
    // 抛出的异常在基类中有处理,都不会推出,只会报错,所以无影响,这里不捕获
    return _get_line(str);
}

int CAccessControl::_get_line(const char * str)
{
    // 配置文件中以冒号分割
    static const char * sep = ":";
    
    char *          tmp = NULL; // 用来做strtok_r的参数
    char *          tok = NULL;
    
    char t[MAX_CONF_LENGTH_SIZE] = {0};
    std::set<int>   request_set;

    // 复制一个临时字符串,不修改str
    strncpy(t, str, sizeof(t) - 1);

    std::string ip;

    // request_type
    tok = strtok_r(t, sep, &tmp);
    if (tok == NULL)
    {
        // 没有按request_type的限制，直接加到ip中即可
        // 第一个应该不会到这里
        ip = t;
    }
    else
    {
        ip = strstrip(tok);
    }

    tok = strtok_r(NULL, sep, &tmp);
    if (tok == NULL)
    {
        // do nothing, request_type set本身就是空
    }
    else
    {
        // 获取request_type列表
        std::vector<std::string> tmp_set;

        split(strstrip(tok), ",", MAX_REQUEST_TYPE_PER_IP,  tmp_set);

        for (std::vector<std::string>::iterator it = tmp_set.begin(); 
                it != tmp_set.end(); ++it)
        {
            trpc_debug_log("ip: %s, tok: %s, key: %s", ip.c_str(), tok, it->c_str());
            request_set.insert(atoi(it->c_str()));
        }
    }

    // 覆盖以前的set
    _access_control[ip] = request_set;
    
    return 0;
}


void CAccessControl::check(const char * ip, const int request_type)
{
    CAutoRLock l(_mutex);

    _check(ip, request_type);
}

void CAccessControl::check(const char * ip)
{
    CAutoRLock l(_mutex);

    _check(ip);
}

void CAccessControl::_check(const char * ip, const int request_type)
{
    if (_access_control.count(ip) <= 0)
    {
        // ip不允许访问 --  错误码为10003 
        THROW(CServerException, InvalidAccess, "ip: %s not allowed to access", ip);
    }

    // 如果有针对request_type的限制，直接做检查
    if (_access_control[ip].size() != 0)
    {
        // 检查是否允许访问对应的request_type
        if (_access_control[ip].find(request_type) == _access_control[ip].end())
        {
            // ip + request_type 不允许访问
            THROW(CServerException, InvalidAccess, "ip: %s, request_type: %d not allowed to access", ip, request_type);
        }
    }
}

void CAccessControl::_check(const char * ip)
{
    if (_access_control.count(ip) <= 0)
    {
        // ip不允许访问 --  错误码为10003 
        THROW(CServerException, InvalidAccess, "ip: %s not allowed to access", ip);
    }
}
    

