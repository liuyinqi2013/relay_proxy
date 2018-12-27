#ifndef     __HASITE_CONF_H__
#define     __HASITE_CONF_H__

#include    <string>
#include    <map>
#include    <vector>

#include    <time.h>

#include    "thread_mutex.h"
//#include    "switch_mgr.h"

#define TIXML_USE_STL

#include    "tinyxml.h"

// server下层服务的协议类型, 暂时支持middle或者relay
enum EM_SERVER_TYPE
{
    EM_ST_MIDDLE = 1,   // middle协议
    EM_ST_RELAY  = 2,   // relay 协议   
};

// haSite -- 保存 hasite.xml
class CHaSiteConf
{ 
public:
    CHaSiteConf();
    ~CHaSiteConf();

    std::string get_next_idc(const char *server_name);
    std::string get_current_idc(const char *server_name); 

    void set_current_idc(const char *server_name);
    
    // 根据服务站点, 查询可用ip
    void get(const char *idc, const char * server_name, std::string& ip, int& port, int& protocol_type);

    void set_valid(const char *idc, const char * server_name, const char * ip, const int port);
    void set_invalid(const char *idc, const char * server_name, const char * ip, const int port);

    void set_invalid_forever(const char *idc, const char * server_name, const char * ip, const int port);

    void read_conf(const char * conf_file);
    void reload(const char * conf_file);

public:
    static int get_protocol_type(const char * protocol_type);

    static const char * default_idc;
    
protected:
    void _get_hasite_conf(const char * conf_file);
    
    void _get_site_info(const TiXmlNode* site_node);
    
    void _get(const char *idc, const char * server_name, std::string& ip, int& port, int& protocol_type);

    void _set_valid(const char *idc, const char * server_name, const char * ip, const int port);

    void _set_invalid(const char *idc, const char * server_name, const char * ip, const int port, bool forever);

protected:
    // 最大的ha site node数量
    static const int MAX_HASITENODE = 5;
    static const int MAX_IDC_NAME_LENGTH = 16;
    
public:
    struct ST_HaSiteNodeInfo
    {
        char    ip[16];
        int     port;

        char    idc[MAX_IDC_NAME_LENGTH];

        bool    is_valid;       // 是否可用
        bool    is_forever_invalid;
        time_t  valid_time; // 失效开始恢复的时间
        time_t  invalid_time;   // 设置为出错的时间
    };
    
    struct ST_HaSiteInfo
    {
        int     protocol_type;
        int     estop_time;     // 发生故障时禁用时间，以秒为单位
        int     hasite_node_num;    // 每个site, ha支持的node数量,最多MAX_HASITENODE个     

        std::vector<std::string>  switch_order_list;
        std::string               current_idc;

        // 支持ha的站点数量,最多5个,现在采用vip的方式,应该足够
        ST_HaSiteNodeInfo   hasite_node[MAX_HASITENODE]; 
    };

protected:
    friend class RelaySwitchMgr;

    // 读写线程锁
    CRwLock     _mutex;
    
    // 保存ip和端口的信息 
    std::map<std::string, ST_HaSiteInfo> _hasite_conf;
};

// 

#endif

