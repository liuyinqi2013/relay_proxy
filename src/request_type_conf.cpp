#include    "request_type_conf.h"
#include    "relay_error.h"
#include <stdlib.h>

// request_type 采取的配置方式为
// 关键字1^关键字2^等作为key,然后拼串路由
const char * CRequestTypeConf::REQUEST_KEY_SPLIT_CHAR = "^";

CRequestTypeConf::CRequestTypeConf()
    :
    CConfig()
{
}

CRequestTypeConf::~CRequestTypeConf()
{
}

void CRequestTypeConf::_clear()
{
    _request_conf.clear();
}

int CRequestTypeConf::get_line(const char * str)
{
    // 抛出的异常在基类中有处理,都不会推出,只会报错,所以无影响,这里不捕获
    return _get_line(str);
}

void CRequestTypeConf::get(const char * request_type, ST_RequestConfData& request_conf)
{
    // 读锁
    CAutoRLock l(_mutex);

    if (_request_conf.count(request_type) == 0)
    {
        // 配置不存在,报错 -- 复用以前的错误码
        THROW(CConfigException, RequestTypeInvalid, "request_type_conf: request_type not find[%s]", request_type);
    }
    
    ST_RequestConfData& d = _request_conf[request_type];

    // 复制现在信息
    memcpy(&request_conf, &d, sizeof(ST_RequestConfData));
}

// 按行读取配置
int CRequestTypeConf::_get_line(const char * str)
{
    // 配置文件中以冒号分割
    /*
    static const char *   sep = ":";
    
    ST_RequestConfData  record;

    char *          tmp = NULL; // 用来做strtok_r的参数
    char *          tok = NULL;
    
    char t[MAX_CONF_LENGTH_SIZE] = {0};
    memset(&record, 0, sizeof(record));
     */

    std::vector<std::string> conf_vec;
    split(str, ":", 8, conf_vec);

    if (conf_vec.size() < 5)
    {
        THROW(CConfigException, ERR_CONF, "lack of some config items[%s]", str);
    }

    std::string master_key;
    std::string sub_keys;
    bool is_multi_key = false;
    size_t pos = conf_vec[0].find_first_of('|');
    if (pos == std::string::npos)
    {
        master_key = conf_vec[0];
    }
    else
    {
        master_key = conf_vec[0].substr(0, pos);
        sub_keys = conf_vec[0].substr(pos+1, conf_vec[0].length());
        is_multi_key = true;
    }

    int property = atoi(conf_vec[1].c_str());
    int site_no = atoi(conf_vec[2].c_str());
    int protocol = atoi(conf_vec[3].c_str());

    if (is_multi_key)
    {
        std::vector<std::string> sub_key_vec;
        std::vector<std::string> service_name_vec;

        split(sub_keys.c_str(), ",", 256, sub_key_vec);
        split(conf_vec[4].c_str(), ",", 256, service_name_vec); 

        if (sub_key_vec.size() != service_name_vec.size())
        {
            THROW(CConfigException, ERR_CONF, "request_type number is not equal service_name number[%s]", str);
        }

        for (int i = 0; i < (int)sub_key_vec.size(); ++i)
        {
            ST_RequestConfData  record;
            memset(&record, 0x00, sizeof(record));
            std::string request_key = master_key + "^" + sub_key_vec[i];
            strncpy(record.request_type, request_key.c_str(), sizeof(record.request_type));
            record.property = property;
            record.site_no = site_no;
            record.protocol = protocol;
            strncpy(record.service_name, service_name_vec[i].c_str(), sizeof(record.service_name));

            _request_conf[request_key] = record;
        }
    }
    else
    {
        ST_RequestConfData  record;
        memset(&record, 0x00, sizeof(record));
        strncpy(record.request_type, master_key.c_str(), sizeof(record.request_type));
        record.property = property;
        record.site_no = site_no;
        record.protocol = protocol;
        strncpy(record.service_name, conf_vec[4].c_str(), sizeof(record.service_name));

       _request_conf[master_key] = record;
    }

    /*
    // 复制一个临时字符串,不修改str
    strncpy(t, str, sizeof(t) - 1);

    // request_type
    tok = strtok_r(t, sep, &tmp);
    if (tok == NULL)
    {
        THROW(CConfigException, ERR_CONF, "request_type_conf: request_type not find[%s]", str);
    }
    else
    {
        strncpy(record.request_type, strstrip(tok), sizeof(record.request_type) - 1);
    }

    // property
    tok = strtok_r(NULL, sep, &tmp);
    if (tok == NULL)
    {
        THROW(CConfigException, ERR_CONF, "request_type_conf: property not find[%s]", str);
    }
    else
    {
        record.property = atoi(strstrip(tok));

        // 校验合法性
        if (record.property <= 0 && record.property > EM_PROPERTY_MAX)
        {
            THROW(CConfigException, ERR_CONF, "request_type_conf: property error: %d", record.property);
        }

    }

    // site_no
    tok = strtok_r(NULL, sep, &tmp);
    if (tok == NULL)
    {
        THROW(CConfigException, ERR_CONF, "request_type_conf: site_no not find[%s]", str);
    }
    else
    {
        record.site_no = atoi(strstrip(tok));
    }

    // protocol type
    tok = strtok_r(NULL, sep, &tmp);
    if (tok == NULL)
    {
        THROW(CConfigException, ERR_CONF, "request_type_conf: protocol type not find[%s]", str);
    }
    else
    {
        record.protocol = atoi(strstrip(tok));

        // 校验合法性
        if (record.protocol <= 0 && record.protocol > EM_PROTOCOL_MAX)
        {
            THROW(CConfigException, ERR_CONF, "request_type_conf: protocol type error: %d", record.protocol);
        }
    }  

    // service name -- for middle
    // or other 
    tok = strtok_r(NULL, sep, &tmp);
    if (tok == NULL)
    {
        THROW(CConfigException, ERR_CONF, "request_type_conf: service_name not find[%s]", str);
    }
    else
    {
        strncpy(record.service_name, strstrip(tok), sizeof(record.service_name) - 1);
    }

    // 检查下此request_type是否存在，如果已存在，报个信息出来
    if (_request_conf.count(record.request_type) > 0)
    {
        trpc_error_log("request_type: %s, also exist !!", record.request_type);    
    }
    
    // 加入map中
    _request_conf[record.request_type] = record;
    */

    // 旧relay的其他统计信息基本已无人使用,线上的允许发送统计信息的配置里面是空的,所以不再读取
    return 0;
}

void
CRequestTypeConf::dump()
{
    for(std::map<std::string, ST_RequestConfData>::const_iterator i = _request_conf.begin();
        i != _request_conf.end();
        ++i)
    {
        const ST_RequestConfData& sr = (*i).second;
        trpc_debug_log("key:%s, property:%d, site_no:%d, protocol:%d, service_name:%s",
                       sr.request_type, sr.property, sr.site_no, sr.protocol, sr.service_name);
    }
}
