#include    "system_conf.h"
#include    "relay_log.h"
#include    "hasite_conf.h"
#include     <stdlib.h>

const char * CSystemConf::REQUEST_TYPE_NAME = "request_type";

CSystemConf::CSystemConf()
    :
    CKeyValueConf()
{
}

CSystemConf::~CSystemConf()
{
}

void CSystemConf::init(const char * file_name)
{
    _init(file_name);
}


void CSystemConf::_init(const char * file_name)
{
    // ��ȡ����
    CKeyValueConf::read_conf(file_name);

    // ��ȡϵͳ���ѡ��
    const char * p = NULL;

    // ip
    p = get_value("ServerIP");
    if (p == NULL)
    {
        THROW(CConfigException, ERR_CONF, "sysconf ServerIP not found");
    }
    else
    {
        _server_ip = p;
    }

    // port
    p = get_value("ServerPort");
    if (p == NULL)
    {
        THROW(CConfigException, ERR_CONF, "sysconf ServerPort not found");
    }
    else
    {
        _server_port = atoi(p);
    }

    p = get_value("RelayMgrServerIP");
    if (p == NULL)
    {
        THROW(CConfigException, ERR_CONF, "sysconf RelayMgrServerIP not found");
    }
    else
    {
        _relay_mgr_ip = p;
    }

    p = get_value("RelayMgrServerPort");
    if (p == NULL)
    {
        THROW(CConfigException, ERR_CONF, "sysconf RelayMgrServerPort not found");
    }
    else
    {
        _relay_mgr_port = atoi(p);
    }

    p = get_value("FetchRuleInterval");
    _fetch_rule_interval = (p == NULL) ? 10 : atoi(p);

    // relay_svr_no
    p = get_value("RelaySvrNo");
    if (p == NULL)
    {
        THROW(CConfigException, ERR_CONF, "sysconf RelaySvrNo not found");
    }
    else
    {
        _relay_svr_no = atoi(p);
    }

    // Զ����־�澯����ip
    p = get_value("RemoteLogIp");
    if (p == NULL)
    {
        THROW(CConfigException, ERR_CONF, "sysconf RemoteLogIp not found");
    }
    else
    {
        _remote_ip = p;
    }

    // Զ����־�澯����port
    p = get_value("RemoteLogPort");
    if (p == NULL)
    {
        THROW(CConfigException, ERR_CONF, "sysconf RemoteLogPort not found");
    }
    else
    {
        _remote_port = atoi(p);
    } 
    
    // thread_num ������ǰ������ ListenThreads
    p = get_value("ListenThreads");
    if (p == NULL)
    {
        THROW(CConfigException, ERR_CONF, "sysconf ListenThreads not found");
    }
    else
    {
        _thread_num = atoi(p);
    } 
    
    // CgiWriteTimeOut ������ǰ������
    p = get_value("CgiWriteTimeOut");
    if (p == NULL)
    {
        // Ĭ��
        _cgi_write_time_out = 500;
    }
    else
    {
        _cgi_write_time_out = atoi(p);
    }

    // connect time out ��ͳһһ��ʱ��
    p = get_value("ConnectTimeOut");
    if (p == NULL)
    {
        // Ĭ��
        _connect_time_out = 5;
    }
    else
    {
        _connect_time_out = atoi(p);
    }

    p = get_value("SwitchAction");
    if (p == NULL)
    {
        _site_switch_action = 0;
    }
    else if (strncasecmp(p, "on", 2) == 0)
    {
        _site_switch_action = 1;
    }
    else
    {
       _site_switch_action = 0;
    }
 
    // ��־·��
    p = get_value("LogPath");
    if (p == NULL)
    {
        _log_path = "/var/log";
    }
    else
    {
        _log_path = p;
    }

    // ��־����
    p = get_value("LogNum");
    if (p == NULL)
    {
        _log_num = 5;
    }
    else
    {
        _log_num = atoi(p);
    }

    // ��־��С���õĵ�λΪM
    p = get_value("LogSize");
    if (p == NULL)
    {
        _log_size = 100 * 1000000;
    }
    else
    {
        _log_size = atoi(p) * 1000000;
    }

    p = CKeyValueConf::get_value("LogWithLock");

    _log_with_lock = (p == NULL || strncmp(p, "on", 2) == 0) ? true : false;

    p = get_value("KEEPALIVE_IDLE");
    _keepalive_idle = (p == NULL) ? 300 : atoi(p);
    if (_keepalive_idle == 0)
    {
        _keepalive_idle = 300;
    }

    // ��������ĳ�ʼ����־
    init_log(log_path(), log_size(), log_num());
    
    // ��ȡ·���õ�request_key
    _get_request_key();
}

void CSystemConf::_get_request_key()
{
    _request_key.clear();
    
    // �Ȼ�ȡ����
    const char * p = CKeyValueConf::get_value("RequestKeyNum");

    if (p == NULL)
    {
        // �����ã���ȡĬ��ֵ
        _set_request_key_default();
    }
    else
    {
        _request_key_num = atoi(p);

        for (int i = 0; i < _request_key_num; ++i)
        {
            _get_request_key_i(i);
        }
    }
}

void CSystemConf::_get_request_key_i(int i)
{
    char request_type_begin_name[64] = {0};
    char request_type_end_name[64] = {0};
    char request_conf_file_name[64] = {0};
    char request_key_name[64] = {0};

    // ������������
    snprintf(request_type_begin_name, sizeof(request_type_begin_name) - 1, "RequestTypeBegin_%d", i);
    snprintf(request_type_end_name, sizeof(request_type_end_name) - 1, "RequestTypeEnd_%d", i);
    snprintf(request_conf_file_name, sizeof(request_conf_file_name) - 1, "RequestKeyAppConf_%d", i);
    snprintf(request_key_name, sizeof(request_key_name) - 1, "RequestKey_%d", i);

    ST_RequestKeyConf st;
    
    // ��ȡ request_key ����Ϣ
    const char * p = CKeyValueConf::get_value(request_type_begin_name);
    if (p == NULL)
    {
        THROW(CConfigException, ERR_CONF, "sysconf : %s not found", request_type_begin_name);
    }
    else
    {
        st.begin = atoi(p);
    }

    p = CKeyValueConf::get_value(request_type_end_name);
    if (p == NULL)
    {
        THROW(CConfigException, ERR_CONF, "sysconf : %s not found", request_type_end_name);
    }
    else
    {
        st.end = atoi(p);
    }

    p = CKeyValueConf::get_value(request_conf_file_name);
    if (p == NULL)
    {
        THROW(CConfigException, ERR_CONF, "sysconf : %s not found", request_conf_file_name);
    }
    else
    {
        st.conf_file = p;
    }

    p = CKeyValueConf::get_value(request_key_name);
    if (p == NULL)
    {
        THROW(CConfigException, ERR_CONF, "sysconf : %s not found", request_key_name);
    }
    else
    {
        split(p, CRequestTypeConf::REQUEST_KEY_SPLIT_CHAR, MAX_REQUEST_KEY_NUM * 2, st.request_key);

        if ((int)st.request_key.size() > MAX_REQUEST_KEY_NUM)
        {
            THROW(CConfigException, ERR_CONF, "sysconf _request_key num error: %lu, buf: %s",
                        st.request_key.size(), p);
        }
    }

    // ��ӡdebug��Ϣ
    trpc_debug_log("request_%d, begin: %d, end: %d, key: %s, conf_file: %s",
                        i, st.begin, st.end, p, st.conf_file.c_str());

    _request_key.push_back(st);
}

void CSystemConf::_set_request_key_default()
{   
    _request_key_num = 1;
    
    ST_RequestKeyConf st;
    st.begin = 0;
    st.end = MAX_REQUEST_TYPE;
    st.conf_file = "./conf/RequestAppServer.conf";
    st.request_key.push_back(REQUEST_TYPE_NAME);

    // ����˺���ǰ�����
    _request_key.push_back(st);
}

bool CSystemConf::is_switch_on() const
{
    return _site_switch_action == 1;
}

std::string 
CSystemConf::master_request_key(int request_type) const
{
    for (std::vector<ST_RequestKeyConf>::const_iterator iter = _request_key.begin();
         iter != _request_key.end();
         ++iter)
    {
        const ST_RequestKeyConf & rc = (*iter);
        if (request_type >= rc.begin && request_type < rc.end)
        {
            return rc.request_key[0];
        }
    }

    return "";
}
