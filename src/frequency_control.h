#ifndef     __FREQUENCY_CONTROL_H__
#define     __FREQUENCY_CONTROL_H__

/*
    以前旧的 FreqLimit.conf
        兼容旧配置，request_type:ip:频率的限制

*/

#include    <map>
#include    <string>

#include    <string.h>

#include    "thread_mutex.h"
#include    "trpc_config.h"


class CFrequencyControl: public CConfig
{
public:
    CFrequencyControl();
    virtual ~CFrequencyControl();
    
    void check(const char * ip, const int request_type);
    
protected:
    virtual int get_line(const char * str);
    virtual void _clear();
    
    int     _get_line(const char * str);

    void    _check(const char * ip, const int request_type);

public:
    static const int MAX_IP_LEN = 32;

public:
    struct ST_FrequencyControlData
    {
        // requet_type -- key不在这里保存
        // int             request_type;
        // char            ip[MAX_IP_LEN];
        int             limit_time; // 限制时间间隔
        int             limit_count;    // 间隔内的次数

        time_t          begin_time;         // 区间的开始时间
        int             call_times;          // 请求总数
    };

    class CFrequencyControlKey
    {
    public:
        CFrequencyControlKey(const int request_type, const char * ip)
        {
            _request_type = request_type;
            strncpy(_ip, ip, sizeof(_ip) - 1);
        }
        
        ~CFrequencyControlKey()
        {
        }
        
        bool operator < (const CFrequencyControlKey& a) const
        {
            if (_request_type < a._request_type)
            {
                return true;
            }
            else if (_request_type > a._request_type)
            {
                return false;
            }

            // request_type 相等
            if (strcmp(_ip, a._ip) < 0)
            {
                return true;
            }

            return false;
        }
        
    protected:
        int     _request_type;
        char    _ip[MAX_IP_LEN];
    };

    typedef std::map<CFrequencyControlKey, ST_FrequencyControlData> CFrequencyMap;

protected:
    CFrequencyMap   _map;
};

#endif

