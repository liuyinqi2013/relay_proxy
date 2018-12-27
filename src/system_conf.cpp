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
    // 读取配置
    CKeyValueConf::read_conf(file_name);

    // 读取系统相关选项
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

    // 远程日志告警服务ip
    p = get_value("RemoteLogIp");
    if (p == NULL)
    {
        THROW(CConfigException, ERR_CONF, "sysconf RemoteLogIp not found");
    }
    else
    {
        _remote_ip = p;
    }

    // 远程日志告警服务port
    p = get_value("RemoteLogPort");
    if (p == NULL)
    {
        THROW(CConfigException, ERR_CONF, "sysconf RemoteLogPort not found");
    }
    else
    {
        _remote_port = atoi(p);
    } 
    
    // thread_num 复用以前的配置 ListenThreads
    p = get_value("ListenThreads");
    if (p == NULL)
    {
        THROW(CConfigException, ERR_CONF, "sysconf ListenThreads not found");
    }
    else
    {
        _thread_num = atoi(p);
    } 
    
    // CgiWriteTimeOut 复用以前的配置
    p = get_value("CgiWriteTimeOut");
    if (p == NULL)
    {
        // 默认
        _cgi_write_time_out = 500;
    }
    else
    {
        _cgi_write_time_out = atoi(p);
    }

    // connect time out 用统一一个时间
    p = get_value("ConnectTimeOut");
    if (p == NULL)
    {
        // 默认
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
 
    // 日志路径
    p = get_value("LogPath");
    if (p == NULL)
    {
        _log_path = "/var/log";
    }
    else
    {
        _log_path = p;
    }

    // 日志数量
    p = get_value("LogNum");
    if (p == NULL)
    {
        _log_num = 5;
    }
    else
    {
        _log_num = atoi(p);
    }

    // 日志大小配置的单位为M
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

    // 尽可能早的初始化日志
    init_log(log_path(), log_size(), log_num());
    
    // 获取路由用的request_key
    _get_request_key();
}

void CSystemConf::_get_request_key()
{
    _request_key.clear();
    
    // 先获取总数
    const char * p = CKeyValueConf::get_value("RequestKeyNum");

    if (p == NULL)
    {
        // 无设置，读取默认值
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

    // 设置配置名字
    snprintf(request_type_begin_name, sizeof(request_type_begin_name) - 1, "RequestTypeBegin_%d", i);
    snprintf(request_type_end_name, sizeof(request_type_end_name) - 1, "RequestTypeEnd_%d", i);
    snprintf(request_conf_file_name, sizeof(request_conf_file_name) - 1, "RequestKeyAppConf_%d", i);
    snprintf(request_key_name, sizeof(request_key_name) - 1, "RequestKey_%d", i);

    ST_RequestKeyConf st;
    
    // 读取 request_key 的信息
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

    // 打印debug信息
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

    // 进入此函数前已清空
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
