#ifndef     __CORE_SERVER_CONF_H__
#define     __CORE_SERVER_CONF_H__

#include    <string>
#include    <vector>
#include    <map>

#include    "thread_mutex.h"
#include    "trpc_config.h"
#include    "api_pool.h"
#include    "hasite_conf.h"
#include    "system_conf.h"
#include    "switch_key_stats.h"
#include    "switch_mgr.h"

class CCoreServerConf: public CConfig
{
public:
    CCoreServerConf();
    ~CCoreServerConf();

    //void    init_api_pool(CHaSiteConf& hasite_conf, CSystemConf& sys_conf, ServiceStatsPool *pool);
    void    init_api_pool(CHaSiteConf& hasite_conf, CSystemConf& sys_conf);

    std::string server_name(int site_no);
    CApiBase * get_api(const int site_no);
    void     free_api(const int site_no, CApiBase * api, bool force_close);

    CApiPool *get_api_pool(const int site_no);

    void     set_switch_on();

    void     re_init(const char *config_file, CHaSiteConf& hasite_conf, CSystemConf& sys_conf);

protected:
    int     _get_line(const char * str);
    
    virtual int get_line(const char * str);

    virtual void _clear();
    
protected:
    static const int MAX_SERVER_NAME = 64;
    static const int MAX_USER_LEN = 33;
    static const int MAX_PASSWD_LEN = 33;
    
public:
    // 按照配置文件中的信息读取
    struct ST_CoreServerConf
    {
        int     site_no;
        int     conn_num;
        int     max_conn_num;
        char    server_name[MAX_SERVER_NAME];   // 对应hasite中的ip和端口信息
        int     port;               // core_server配置中的port，已不再使用，先读取信息
        int     time_out;
        
        // 以前为middle服务特用信息，沿用以前配置
        char    user[MAX_USER_LEN];
        char    passwd[MAX_PASSWD_LEN];
    };

public:
    void get(int site_no, ST_CoreServerConf& core_server_conf);
    
protected:

    friend class RelaySwitchMgr;
    // 和以前一样，采用int作为主键
    std::map<int, ST_CoreServerConf>    _core_server_conf;

    // api连接池单独存放
    std::map<int, CApiPool *>   _api_pool;
};

#endif

