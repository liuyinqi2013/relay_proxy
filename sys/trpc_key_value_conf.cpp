#include    "trpc_sys_exception.h"
#include    "trpc_key_value_conf.h"
#include <stdlib.h>

CKeyValueConf::CKeyValueConf()
    :
    CConfig()
{
}

CKeyValueConf::~CKeyValueConf()
{
}

int CKeyValueConf::comp(const void * p1, const void * p2)
{    
    const key_value_info * item1 = (const key_value_info *)p1;
    const key_value_info * item2 = (const key_value_info *)p2;

    return strncmp(item1->key, item2->key, sizeof(item1->key));
}

int CKeyValueConf::search_comp(const void * p1, const void * p2)
{
    const char * item1 = (const char *)p1;
    const key_value_info * item2 = (const key_value_info *)p2;

    return strncmp(item1, item2->key, sizeof(item2->key));
}

void CKeyValueConf::clear()
{
    CConfig::clear();
    
    _data.clear();
}

int CKeyValueConf::get_line(const char * str)
{
    // 分隔符是':'
    static const char *   sep = ":";

    key_value_info  record;
    memset(&record, 0x00, sizeof(record));

    char *          tmp = NULL; // 用来做strtok_r的参数
    char *          tok = NULL;
    
    char t[MAX_CONF_LENGTH_SIZE] = {0};
    memset(&record, 0, sizeof(record));

    // 复制一个临时字符串,不修改str
    strncpy(t, str, sizeof(t) - 1);
    
    // get key
    tok = strtok_r(t, sep, &tmp);
    if (tok == NULL)
    {
        // 应该不会出现,第一次返回原串第一个地址
        THROW(CConfigException, ERR_CONF, "CKeyValueConf: error");
    }
    else
    {
        strncpy(record.key, strstrip(tok), sizeof(record.key) - 1);
    }
    
    // get value
    tok = strtok_r(NULL, sep, &tmp);
    if (tok == NULL)
    {
        // 没有分隔符,先报错 -- 但是进程不停，后续继续处理
        THROW(CConfigException, ERR_CONF, "CKeyValueConf: sep [=] not find");
    }
    else
    {
        strncpy(record.value, strstrip(tok), sizeof(record.value) - 1);
    }
    
    _data.push_back(record);

    return 0;
}

void CKeyValueConf::read_conf(const char * conf_file)
{
    // 调用父类的函数
    CConfig::read_conf(conf_file);

    // 进行排序
    qsort(&_data[0], _data.size(), sizeof(key_value_info), comp);
}

// 获取文件中的配置项
const char * CKeyValueConf::get_value(const char * key)
{
    const key_value_info * p = (const key_value_info *)bsearch(key, 
                &_data[0], _data.size(), 
                sizeof(key_value_info), search_comp);

    if (p == NULL)
    {
        return NULL;
    }

    return p->value;
}

