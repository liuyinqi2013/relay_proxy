#include    "core_server_conf.h"
#include    "relay_error.h"
#include    "trpc_package.h"

CCoreServerConf::CCoreServerConf()
    :
    CConfig()
{
}

CCoreServerConf::~CCoreServerConf()
{
}

void CCoreServerConf::_clear()
{
    _core_server_conf.clear();
}

std::string
CCoreServerConf::server_name(int site_no)
{
    CAutoRLock l(_mutex);

    if (_core_server_conf.count(site_no) == 0)
    {
        // 配置不存在,报错 -- 复用以前的错误码
        THROW(CConfigException, TrpcInfoPoolInvalid, "core_server: site_no not find[%d]", site_no);
    }
    
    ST_CoreServerConf& d = _core_server_conf[site_no];

    return d.server_name;
}

void CCoreServerConf::get(int site_no, ST_CoreServerConf& core_server_conf)
{
    CAutoRLock l(_mutex);

    if (_core_server_conf.count(site_no) == 0)
    {
        // 配置不存在,报错 -- 复用以前的错误码
        THROW(CConfigException, TrpcInfoPoolInvalid, "core_server: site_no not find[%d]", site_no);
    }
    
    ST_CoreServerConf& d = _core_server_conf[site_no];

    // 复制现在信息
    memcpy(&core_server_conf, &d, sizeof(ST_CoreServerConf));
}

int CCoreServerConf::get_line(const char * str)
{
    return _get_line(str);
}


int CCoreServerConf::_get_line(const char * str)
{
    // 配置文件中以逗号分割
    static const char *   sep = ",";
    
    ST_CoreServerConf  record;

    char *          tmp = NULL; // 用来做strtok_r的参数
    char *          tok = NULL;
    
    char t[MAX_CONF_LENGTH_SIZE] = {0};
    memset(&record, 0, sizeof(record));

    // 复制一个临时字符串,不修改str
    strncpy(t, str, sizeof(t) - 1);

    // site_no
    tok = strtok_r(t, sep, &tmp);
    if (tok == NULL)
    {
        THROW(CConfigException, ERR_CONF, "core_server_conf: site_no not find[%s]", str);
    }
    else
    {
        record.site_no = atoi(strstrip(tok));
    }

    // conn_num
    tok = strtok_r(NULL, sep, &tmp);
    if (tok == NULL)
    {
        THROW(CConfigException, ERR_CONF, "core_server_conf: conn_num not find[%s]", str);
    }
    else
    {
        record.conn_num = atoi(strstrip(tok));
    }

    // max_conn_num
    tok = strtok_r(NULL, sep, &tmp);
    if (tok == NULL)
    {
        THROW(CConfigException, ERR_CONF, "core_server_conf: max_conn_num not find[%s]", str);
    }
    else
    {
        record.max_conn_num= atoi(strstrip(tok));
    }

    // server_name
    tok = strtok_r(NULL, sep, &tmp);
    if (tok == NULL)
    {
        THROW(CConfigException, ERR_CONF, "core_server_conf: server_name not find[%s]", str);
    }
    else
    {
        strncpy(record.server_name, strstrip(tok), sizeof(record.server_name) - 1);
    }  

    // port
    tok = strtok_r(NULL, sep, &tmp);
    if (tok == NULL)
    {
        THROW(CConfigException, ERR_CONF, "core_server_conf: port not find[%s]", str);
    }
    else
    {
        record.port = atoi(strstrip(tok));
    }

    // time out
    tok = strtok_r(NULL, sep, &tmp);
    if (tok == NULL)
    {
        THROW(CConfigException, ERR_CONF, "core_server_conf: time out not find[%s]", str);
    }
    else
    {
        record.time_out = atoi(strstrip(tok));
    }

    // user
    tok = strtok_r(NULL, sep, &tmp);
    if (tok == NULL)
    {
        THROW(CConfigException, ERR_CONF, "core_server_conf: user not find[%s]", str);
    }
    else
    {
        strncpy(record.user, strstrip(tok), sizeof(record.user) - 1);
    }  

    // passwd
    tok = strtok_r(NULL, sep, &tmp);
    if (tok == NULL)
    {
        THROW(CConfigException, ERR_CONF, "core_server_conf: passwd not find[%s]", str);
    }
    else
    {
        strncpy(record.passwd, strstrip(tok), sizeof(record.passwd) - 1);
    }  

    // 加入map中
    _core_server_conf[record.site_no] = record; 

    return 0;
}

//void CCoreServerConf::init_api_pool(CHaSiteConf& hasite_conf, CSystemConf& sys_conf, ServiceStatsPool *pool)
void CCoreServerConf::init_api_pool(CHaSiteConf& hasite_conf, CSystemConf& sys_conf)
{
    CAutoWLock l(_mutex);
    
    for (std::map<int, ST_CoreServerConf>::iterator it = _core_server_conf.begin();
            it != _core_server_conf.end(); ++it)
    {
        ST_CoreServerConf& conf = it->second;
        
        CApiPool * p = new CApiPool(hasite_conf, sys_conf, conf.server_name, conf.conn_num, conf.max_conn_num, conf.time_out);

        if (p == NULL)
        {
            THROW(CTrpcException, ERR_SYS_UNKNOWN, "new api pool error server_name: %s, site_no: %d",
                        conf.server_name, conf.site_no);
        }

        //p->set_service_stats_pool(pool);

        // 加入链接池集合中
        _api_pool[conf.site_no] = p;
    }

    // init over
}

CApiBase * CCoreServerConf::get_api(const int site_no)
{
    CAutoRLock l(_mutex);
    
    if (_api_pool.find(site_no) == _api_pool.end())
    {
        THROW(CConfigException, ERR_CONF, "site_no: %d error", site_no);
    }

    ST_CoreServerConf &  core_server = _core_server_conf[site_no];

    trpc_debug_log("TRPC TO MIDDLE, USER:%s, PASSWD:%s\n", core_server.user, core_server.passwd);

    return _api_pool[site_no]->get(core_server.user, core_server.passwd);
}

CApiPool *
CCoreServerConf::get_api_pool(int site_no)
{
    CAutoRLock l(_mutex);
    
    return _api_pool[site_no];
}

void CCoreServerConf::free_api(const int site_no, CApiBase * api, bool force_close)
{
    CAutoRLock l(_mutex);
    
    assert(api != NULL);
    
    if (_api_pool.find(site_no) == _api_pool.end())
    {
        THROW(CConfigException, ERR_CONF, "site_no: %d error", site_no);
    }
    
    _api_pool[site_no]->release(api, force_close);
}


void
CCoreServerConf::set_switch_on()
{
    CAutoWLock l(_mutex);
    
    std::set<std::string> server_set;

    std::map<int, CApiPool *>::iterator iter = _api_pool.begin(), iend = _api_pool.end();
    for (; iter != iend; ++iter)
    {
        CApiPool * &pool = (*iter).second;
        if (server_set.count(pool->server_name()) == 0)
        {
            trpc_write_statslog("set server:%s to be switch\n", pool->server_name().c_str());
            pool->set_switch_on();

            server_set.insert(pool->server_name());
        }
    }
}

void     
CCoreServerConf::re_init(const char *config_file, CHaSiteConf& hasite_conf, CSystemConf& sys_conf)
{
    CAutoWLock l(_mutex);

    _core_server_conf.clear();

    read_conf(config_file);

    for (std::map<int, ST_CoreServerConf>::iterator iter = _core_server_conf.begin();
         iter != _core_server_conf.end();
         ++iter)
    {
        int site_no = (*iter).first;
        std::map<int, CApiPool *>::iterator p_iter = _api_pool.find(site_no);
        if (p_iter == _api_pool.end())
        {
            ST_CoreServerConf & csc = (*iter).second;
            CApiPool * p = new CApiPool(hasite_conf, sys_conf, csc.server_name, csc.conn_num, csc.max_conn_num, csc.time_out);
            _api_pool.insert(std::map<int, CApiPool *>::value_type(site_no, p));
        }
    }
}
