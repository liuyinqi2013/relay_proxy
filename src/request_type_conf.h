#ifndef     __REQUEST_TYPE_CONF_H__
#define     __REQUEST_TYPE_CONF_H__

#include    <string>
#include    <vector>
#include    <map>

#include    "thread_mutex.h"
#include    "trpc_config.h"

enum EM_Property
{
    EM_PROPERTY_DISPATCH = 1,   // 直接转发
    EM_PROPERTY_PROCESS  = 2,   // 非直接转发  
    EM_PROPERTY_MAX,            // 配置最大值,校验合法性用
};

enum EM_Protocol
{
    EM_PROTOCOL_TEXT    = 1,    // 文本协议
    EM_PROTOCOL_BIN     = 2,    // 二进制协议
    EM_PROTOCOL_MAX,            // 配置最大值,校验合法性用
};

class CRequestTypeConf: public CConfig
{
public:
    CRequestTypeConf();
    virtual ~CRequestTypeConf();

    void  dump();

protected:
    virtual int get_line(const char * str);
    virtual void _clear();
    
    int     _get_line(const char * str);
    
public:
    // request_type 采取的配置方式为
    // 关键字1^关键字2^等作为key,然后拼串路由
    static const char * REQUEST_KEY_SPLIT_CHAR;
    
    // 配置文件中以冒号分割
    static const char * REQUEST_SPLIT_CHAR;
    
    // 最大的request_type key的长度 128位应该足够,一般就是服务名字+请求类型
    static const int MAX_REQUEST_TYPE_LEN = 64;
    
    // 最大service_name
    static const int MAX_SERVICE_NAME = 64;

public:
    struct ST_RequestConfData
    {
        char    request_type[MAX_REQUEST_TYPE_LEN];
        int     property;
        int     site_no;
        int     protocol;
        char    service_name[MAX_SERVICE_NAME];
        // 其他的旧relay的信息已不需要再使用,线上其实也不再发送统计信息
    };

public:
    // 根据request_type信息,获取配置
    void get(const char * request_type, ST_RequestConfData& request_conf);

protected:
    std::map<std::string, ST_RequestConfData>   _request_conf;
};

#endif

