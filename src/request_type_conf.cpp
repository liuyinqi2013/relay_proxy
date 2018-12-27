#include    "request_type_conf.h"
#include    "relay_error.h"
#include <stdlib.h>

// request_type ��ȡ�����÷�ʽΪ
// �ؼ���1^�ؼ���2^����Ϊkey,Ȼ��ƴ��·��
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
    // �׳����쳣�ڻ������д���,�������Ƴ�,ֻ�ᱨ��,������Ӱ��,���ﲻ����
    return _get_line(str);
}

void CRequestTypeConf::get(const char * request_type, ST_RequestConfData& request_conf)
{
    // ����
    CAutoRLock l(_mutex);

    if (_request_conf.count(request_type) == 0)
    {
        // ���ò�����,���� -- ������ǰ�Ĵ�����
        THROW(CConfigException, RequestTypeInvalid, "request_type_conf: request_type not find[%s]", request_type);
    }
    
    ST_RequestConfData& d = _request_conf[request_type];

    // ����������Ϣ
    memcpy(&request_conf, &d, sizeof(ST_RequestConfData));
}

// ���ж�ȡ����
int CRequestTypeConf::_get_line(const char * str)
{
    // �����ļ�����ð�ŷָ�
    /*
    static const char *   sep = ":";
    
    ST_RequestConfData  record;

    char *          tmp = NULL; // ������strtok_r�Ĳ���
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
    // ����һ����ʱ�ַ���,���޸�str
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

        // У��Ϸ���
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

        // У��Ϸ���
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

    // ����´�request_type�Ƿ���ڣ�����Ѵ��ڣ�������Ϣ����
    if (_request_conf.count(record.request_type) > 0)
    {
        trpc_error_log("request_type: %s, also exist !!", record.request_type);    
    }
    
    // ����map��
    _request_conf[record.request_type] = record;
    */

    // ��relay������ͳ����Ϣ����������ʹ��,���ϵ�������ͳ����Ϣ�����������ǿյ�,���Բ��ٶ�ȡ
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
