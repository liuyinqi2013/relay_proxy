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
    // ���������ļ��е���Ϣ��ȡ
    struct ST_CoreServerConf
    {
        int     site_no;
        int     conn_num;
        int     max_conn_num;
        char    server_name[MAX_SERVER_NAME];   // ��Ӧhasite�е�ip�Ͷ˿���Ϣ
        int     port;               // core_server�����е�port���Ѳ���ʹ�ã��ȶ�ȡ��Ϣ
        int     time_out;
        
        // ��ǰΪmiddle����������Ϣ��������ǰ����
        char    user[MAX_USER_LEN];
        char    passwd[MAX_PASSWD_LEN];
    };

public:
    void get(int site_no, ST_CoreServerConf& core_server_conf);
    
protected:

    friend class RelaySwitchMgr;
    // ����ǰһ��������int��Ϊ����
    std::map<int, ST_CoreServerConf>    _core_server_conf;

    // api���ӳص������
    std::map<int, CApiPool *>   _api_pool;
};

#endif

