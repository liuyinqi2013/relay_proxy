#ifndef     __SYSTEM_CONF_H__
#define     __SYSTEM_CONF_H__

#include    <string>
#include    <vector>
#include    <map>
#include    <set>
#include    <algorithm>

#include    "thread_mutex.h"
#include    "trpc_config.h"
#include    "request_type_conf.h"
#include    "trpc_key_value_conf.h"

// sys conf ��֧�ֶ�̬���أ���ʱ���ṩ��̬���ؽӿ�
class CSystemConf: public CKeyValueConf
{
public:
    struct ST_RequestKeyConf
    {
        int     begin;
        int     end;
        std::string conf_file;
        std::vector<std::string> request_key;
    };
    
public:
    CSystemConf();
    virtual ~CSystemConf();

    void    init(const char * file_name);

public:
    // get func
    const char * server_ip()
    {
        return _server_ip.c_str();
    };

    const int   server_port()
    {
        return _server_port;
    }

    const char * relay_mgr_ip() const
    {
        return _relay_mgr_ip.c_str();
    }

    int relay_mgr_port() const
    {
        return _relay_mgr_port;
    }

    int fetch_rule_interval() const
    { 
        return _fetch_rule_interval;
    }

    const int   relay_svr_no()
    {
        return _relay_svr_no;
    }

    const char * remote_ip()
    {
        return _remote_ip.c_str();
    }

    const int   remote_port()
    {
        return _remote_port;
    }

    const int   thread_num()
    {
        return _thread_num;
    }

    const int   cgi_write_time_out()
    {
        return _cgi_write_time_out;
    }

    const int   connect_time_out()
    {
        return _connect_time_out;
    }

    const int   get_request_key_num()
    {
        return _request_key.size();
    }

    const ST_RequestKeyConf& get_request_key(int key_index)
    {
        if (key_index < 0 || key_index >= _request_key_num)
        {
            THROW(CTrpcException, ERR_SYS_UNKNOWN, "request_key_index error: %d", key_index);
        }
        
        return _request_key[key_index];
    }

    const char * log_path()
    {
        return _log_path.c_str();
    }

    const int log_size()
    {
        return _log_size;
    }

    const int log_num()
    {
        return _log_num;
    }

    bool log_with_lock() const
    {
        return _log_with_lock;
    }

    const std::vector<ST_RequestKeyConf>& request_key()
    {
        return _request_key;
    }

    bool is_switch_on() const;

    int keepalive_idle() const
    {
        return _keepalive_idle;
    }

    std::string master_request_key(int request_type) const;

protected:
    void    _init(const char * file_name);

    void    _get_request_key();
    void    _get_request_key_i(int i);
    void    _set_request_key_default();
    
public:
    static const int    MAX_REQUEST_KEY_NUM = 5;    // ���֧��5��key, �ٶ��Ȳ�����
    static const int    MAX_CONF_FILE_LENGTH = 128; // �����ļ�����󳤶ȣ�128�㹻�����·��
    static const int    MAX_REQUEST_TYPE = 999999999;   // request_typeĬ�����ֵ��Ӧ�ò����ܳ���
    static const char * REQUEST_TYPE_NAME;
    
// ����system����
protected:
    std::string     _server_ip;     // �󶨵�ip
    int             _server_port;   // �󶨵Ķ˿�

    int             _relay_svr_no; // �����ţ�����MSG_NOʹ��

    std::string     _remote_ip;     // Զ����־ip -- ��ʵΪ��ǰ�ı����˿�
    int             _remote_port;   // Զ����־port

    int             _thread_num;    // ������߳����� -- ������ǰ������ ListenThreads

    int             _cgi_write_time_out;    // cgiд��ʱʱ��
    int             _connect_time_out;

    int             _fetch_rule_interval;

    // ����·��
    std::string     _log_path;
    // ��־����
    int             _log_num;
    // ��־��С
    int             _log_size;

    bool            _log_with_lock;

    int             _request_key_num;
    std::vector<ST_RequestKeyConf>    _request_key;   // request_type��key�������б�

    std::vector<std::string>  _site_switch_order;
    int                       _site_switch_action;

    std::string     _relay_mgr_ip;
    int             _relay_mgr_port;

    int             _keepalive_idle;
};

#endif

