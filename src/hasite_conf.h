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

// server�²�����Э������, ��ʱ֧��middle����relay
enum EM_SERVER_TYPE
{
    EM_ST_MIDDLE = 1,   // middleЭ��
    EM_ST_RELAY  = 2,   // relay Э��   
};

// haSite -- ���� hasite.xml
class CHaSiteConf
{ 
public:
    CHaSiteConf();
    ~CHaSiteConf();

    std::string get_next_idc(const char *server_name);
    std::string get_current_idc(const char *server_name); 

    void set_current_idc(const char *server_name);
    
    // ���ݷ���վ��, ��ѯ����ip
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
    // ����ha site node����
    static const int MAX_HASITENODE = 5;
    static const int MAX_IDC_NAME_LENGTH = 16;
    
public:
    struct ST_HaSiteNodeInfo
    {
        char    ip[16];
        int     port;

        char    idc[MAX_IDC_NAME_LENGTH];

        bool    is_valid;       // �Ƿ����
        bool    is_forever_invalid;
        time_t  valid_time; // ʧЧ��ʼ�ָ���ʱ��
        time_t  invalid_time;   // ����Ϊ�����ʱ��
    };
    
    struct ST_HaSiteInfo
    {
        int     protocol_type;
        int     estop_time;     // ��������ʱ����ʱ�䣬����Ϊ��λ
        int     hasite_node_num;    // ÿ��site, ha֧�ֵ�node����,���MAX_HASITENODE��     

        std::vector<std::string>  switch_order_list;
        std::string               current_idc;

        // ֧��ha��վ������,���5��,���ڲ���vip�ķ�ʽ,Ӧ���㹻
        ST_HaSiteNodeInfo   hasite_node[MAX_HASITENODE]; 
    };

protected:
    friend class RelaySwitchMgr;

    // ��д�߳���
    CRwLock     _mutex;
    
    // ����ip�Ͷ˿ڵ���Ϣ 
    std::map<std::string, ST_HaSiteInfo> _hasite_conf;
};

// 

#endif

